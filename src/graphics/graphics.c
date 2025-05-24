
#include "raylib.h"
#include "config.h"
#include "game.h"
#include "gang.h"
#include "shared_mem_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


/* Window & layout --------------------------------------------------*/
#define WIN_W 1400
#define WIN_H 800
#define SIDE_W 260
#define POL_W  220
#define POL_X  SIDE_W
#define PAD    10
#define COL_BG (Color){245,245,245,255}


static Rectangle R_AGT = {0, 0,            SIDE_W, 470};
static Rectangle R_GME = {0, WIN_H-150,    SIDE_W, 150};
static Rectangle R_POL = {POL_X, 0,        POL_W , WIN_H};
static Rectangle R_GAN = {POL_X+POL_W+4,0, WIN_W-(POL_X+POL_W+4), WIN_H};

/* card geometry */
#define CARD_W 260.f
#define BASE_CARD_H 150.f  // base height without members
#define MEM_R  14.f
#define ROWS   2
static float hScroll = 0.f;
static float vScroll = 0.f;  // vertical scroll for gang panel
static Texture2D texGang;


 static Texture2D mustLoad(const char *p){
     Texture2D t = LoadTexture(p);
     if(!t.id){ TraceLog(LOG_FATAL,"cannot load %s",p); exit(1); }
     return t;
 }

/* ------------------------------------------------ helpers ---------*/
static const char* target_name(TargetType t){
    static const char* n[]={"Bank","Jewelry","Drugs","Art","Kidnap",
                            "Blackmail","Arms"};
    return (t<NUM_TARGETS)? n[t] : "U";
}
static void panel(Rectangle r,const char*s){
    DrawRectangleLinesEx(r,2,GRAY);
    DrawText(s,r.x+PAD,r.y+PAD,20,DARKBLUE);
}
static void draw_member(float cx,float cy,const Member*m){
    float texSize = 28;
    Rectangle dest = {cx - texSize/2, cy - texSize/2, texSize, texSize};
    DrawTexturePro(texGang, (Rectangle){0, 0, texGang.width, texGang.height}, dest,
                   (Vector2){0, 0}, 0, WHITE);

    // Rank overlaid
    DrawText(TextFormat("%d", m->rank), cx - 6, cy - 6, 12, BLACK);

    // Progress bar
    float barW = texSize, barY = cy + texSize/2 + 2;
    float pct = (m->prep_contribution > 100) ? 100.0f : m->prep_contribution;
    Color c = (pct > 70) ? DARKGREEN : (pct > 30 ? ORANGE : RED);
    DrawRectangle(cx - texSize/2, barY, barW, 5, GRAY);
    DrawRectangle(cx - texSize/2, barY, barW * (pct / 100.f), 5, c);

    // Percentage below
    DrawText(TextFormat("%.0f%%", pct), cx - 10, barY + 8, 10, BLACK);
}

/* ------------------------------------------------ pointer-fix ---- */
static void fix_snapshot(Game* snap,const Config* cfg)
{
    /* gangs pointer */
    char* base=(char*)snap;
    snap->gangs=(Gang*)(base+sizeof(Game));

    /* members pointers */
    size_t gangs_size   = cfg->max_gangs*sizeof(Gang);
    size_t mem_size_one = cfg->max_gang_size*sizeof(Member);
    for(int i=0;i<cfg->max_gangs;i++){
        snap->gangs[i].members = (Member*)(base+sizeof(Game)+
                                           gangs_size+i*mem_size_one);
    }
}

/* ------------------------------------------------ UI boxes --------*/
static void box_agents(Rectangle r,const Game* g,const Config* cfg){
    panel(r,"Agents");
    int total=0; for(int i=0;i<cfg->max_gangs;i++) total+=g->gangs[i].num_agents;
    int y=r.y+40;
    DrawText(TextFormat("Active   : %d",total),        r.x+PAD,y,18,BLACK); y+=22;
    DrawText(TextFormat("Executed : %d",g->num_executed_agents),
                         r.x+PAD,y,18,BLACK);
}
static void box_game(Rectangle r,const Game* g,const Config* c){
    panel(r,"Game");
    int y=r.y+40;
    DrawText(TextFormat("Elapsed  : %d s",g->elapsed_time),
             r.x+PAD,y,18,BLACK); y+=24;
    DrawText(TextFormat("Success  : %d/%d",g->num_successfull_plans,
                        c->max_successful_plans),        r.x+PAD,y,16,BLACK); y+=18;
    DrawText(TextFormat("Thwarted : %d/%d",g->num_thwarted_plans,
                        c->max_thwarted_plans),          r.x+PAD,y,16,BLACK); y+=18;
    DrawText(TextFormat("Executed : %d/%d",g->num_executed_agents,
                        c->max_executed_agents),         r.x+PAD,y,16,BLACK);
}
static void box_police(Rectangle r){ panel(r,"Police"); }

/* active-gang collection (ignores unreliable total_gangs) */
static int collect_active(const Game* g,const Config* cfg,int idx[]){
    int n=0;
    for(int i=0;i<cfg->max_gangs;i++){
        const Gang* gn=&g->gangs[i];
        if(gn->members_count>0||gn->plan_in_progress||gn->num_agents>0)
            idx[n++]=i;
    }
    return n;
}

/* gang cards */
static void box_gangs(Rectangle r, const Game* g, const Config* cfg) {
    panel(r, "Gangs");

    // Scrolling
    hScroll += (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 12;
    if (hScroll < 0) hScroll = 0;

    vScroll -= GetMouseWheelMove() * 40;
    if (vScroll < 0) vScroll = 0;

    int idx[cfg->max_gangs];
    int total = collect_active(g, cfg, idx);

    float cardX = r.x + PAD;
    float cardY = r.y + 70 - vScroll;
    float maxRight = r.x + r.width;

    Rectangle view = {r.x + 1, r.y + 70, r.width - 2, r.height - 71};
    BeginScissorMode(view.x, view.y, view.width, view.height);

    if (total == 0) {
        DrawText("No gangs yet …", view.x + PAD, view.y + 20, 22, GRAY);
    } else {
        for (int k = 0; k < total; k++) {
            int gi = idx[k];
            const Gang* gn = &g->gangs[gi];

            int members = gn->members_count;
            int perRow = 4;
            int rowCount = (members + perRow - 1) / perRow;
            float cardH = 150 + rowCount * 90.0f;

            if (cardY > r.y + r.height) break;  // skip if below view

            DrawRectangle(cardX, cardY, CARD_W - 12, cardH - 10, (Color){235, 235, 250, 255});
            DrawRectangleLines(cardX, cardY, CARD_W - 12, cardH - 10, GRAY);

            float tx = cardX + 10, ty = cardY + 10;

            DrawText(TextFormat("Gang %d | %s", gi, target_name(gn->target_type)),
                     tx, ty, 18, DARKBLUE); ty += 28;

            const char* st = "Preparing";
            if (gn->plan_in_progress) st = "Executing";
            else if (gn->members_ready == gn->members_count && gn->members_count)
                st = "Ready";

            DrawText(TextFormat("State   : %-9s", st), tx, ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("Ready   : %d/%d", gn->members_ready, gn->members_count),
                     tx, ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("Success : %d", gn->num_successful_plans),
                     tx, ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("Thwart  : %d", gn->num_thwarted_plans),
                     tx, ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("ExecAg  : %d", gn->num_executed_agents),
                     tx, ty, 14, BLACK); ty += 20;

            // Draw members
            const Member* mem = gn->members;
            for (int m = 0; m < members; m++) {
                float mx = tx + 5 + (m % perRow) * 64.0f;
                float my = ty + (m / perRow) * 80.0f;
                draw_member(mx, my, &mem[m]);
            }

            cardY += cardH + 10;
        }
    }

    EndScissorMode();

    // Optional: draw vertical scrollbar if content overflows
    float totalHeight = total * (300.0f); // estimated max card height
    if (totalHeight > view.height) {
        float ratio = view.height / totalHeight;
        float bw = ratio * view.height;
        float by = view.y + (vScroll / totalHeight) * (view.height - bw);
        DrawRectangle(view.x + view.width - 6, by, 4, bw, DARKGRAY);
    }
}

/* -----------------------------------------------------------------*/
int main(void)
{
    /* load configuration (disk) */
    Config cfg;
    if(load_config(CONFIG_PATH,&cfg)==-1){
        fprintf(stderr,"viewer: cannot load config\n");
        return 1;
    }

    /* --- open shared memory read-only --- */
    size_t gameSz = sizeof(Game);
    size_t gangsSz = cfg.max_gangs*sizeof(Gang);
    size_t memSz  = cfg.max_gangs*cfg.max_gang_size*sizeof(Member);
    size_t totalSz= gameSz+gangsSz+memSz;

    int fd = shm_open(GAME_SHM_NAME,O_RDONLY,0);
    if(fd==-1){ perror("viewer: shm_open"); return 1; }

    void* shm = mmap(NULL,totalSz,PROT_READ,MAP_SHARED,fd,0);
    if(shm==MAP_FAILED){ perror("viewer: mmap"); return 1; }
    close(fd);

    const Game* gshm = (const Game*)shm;

    /* allocate snapshot buffer */
    Game* snap = malloc(totalSz);
    if(!snap){ fprintf(stderr,"viewer: malloc snapshot\n"); return 1; }

    /* init raylib */
    InitWindow(WIN_W,WIN_H,"Bakery Gang Viewer (read-only)");
    texGang = mustLoad(ASSETS_PATH "gang.png");
if (texGang.id == 0) {
    fprintf(stderr, "ERROR: Failed to load assets/gang.png\n");
    return 1;
}
    SetTargetFPS(60);

    while(!WindowShouldClose()){
        memcpy(snap,gshm,totalSz);
        fix_snapshot(snap,&cfg);

        BeginDrawing();
          ClearBackground(COL_BG);
          box_police (R_POL);
          box_agents (R_AGT,snap,&cfg);
          box_game   (R_GME,snap,&cfg);
          box_gangs  (R_GAN,snap,&cfg);
        EndDrawing();
    }

    UnloadTexture(texGang);
    CloseWindow();
    munmap(shm,totalSz);
    free(snap);
    return 0;
}
