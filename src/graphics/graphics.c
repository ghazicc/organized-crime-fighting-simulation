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
static Texture2D texAgent;
static Texture2D texPolice;

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
    DrawText(s,(int)(r.x+PAD),(int)(r.y+PAD),20,DARKBLUE);
}
static void draw_member(float cx,float cy,const Member*m){
    // Draw member avatar (fallback: red circle)
    DrawCircle((int)cx, (int)cy, 18, (Color){220, 60, 60, 255});

    // Draw rank
    DrawText(TextFormat("%d", m->rank), (int)(cx - 8), (int)(cy - 8), 16, BLACK);

    // Progress bar
    float texSize = 36;
    float barW = texSize, barY = cy + texSize/2 + 4;
    float pct = (m->prep_contribution > 100) ? 100.0f : (float)m->prep_contribution;
    Color c = (pct > 70) ? DARKGREEN : (pct > 30 ? ORANGE : RED);
    DrawRectangle((int)(cx - texSize/2), (int)barY, (int)barW, 6, GRAY);
    DrawRectangle((int)(cx - texSize/2), (int)barY, (int)(barW * (pct / 100.f)), 6, c);
    DrawText(TextFormat("%.0f%%", pct), (int)(cx - 12), (int)(barY + 8), 12, BLACK);

    // Draw knowledge, suspicion, faithfulness
    int infoY = (int)(barY + 26);
    DrawText(TextFormat("K:%.1f", m->knowledge), (int)(cx - 30), infoY, 10, (Color){40,40,120,255});
    DrawText(TextFormat("S:%.1f", m->suspicion), (int)(cx - 5), infoY, 10, (Color){120,40,40,255});
    DrawText(TextFormat("F:%.1f", m->faithfulness), (int)(cx + 20), infoY, 10, (Color){40,120,40,255});
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
static void box_agents(Rectangle r, const Game* g, const Config* cfg) {
    panel(r, "Agents");
    static float agentScroll = 0.f;
    int y0 = (int)r.y + 40;
    int agent_count = 0;
    int agent_icon_size = 48;
    int gap = 12;
    int x0 = (int)r.x + PAD;
    int max_per_row = 3;
    int col = 0;
    int x = x0;
    int y = y0 - (int)agentScroll;
    // Count total agents for scroll calculation
    for (int i = 0; i < cfg->num_gangs; i++) {
        const Gang* gn = &g->gangs[i];
        for (int j = 0; j < gn->max_member_count; j++) {
            const Member* m = &gn->members[j];
            if (m->is_alive && m->agent_id >= 0) agent_count++;
        }
    }
    int row_count = (agent_count + max_per_row - 1) / max_per_row;
    int content_height = row_count * (agent_icon_size + gap + 40);
    Rectangle view = {r.x + 1, r.y + 36, r.width - 2, r.height - 37};
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
    x = x0; y = y0 - (int)agentScroll;
    int shown = 0;
    col = 0;
    for (int i = 0; i < cfg->num_gangs; i++) {
        const Gang* gn = &g->gangs[i];
        for (int j = 0; j < gn->max_member_count; j++) {
            const Member* m = &gn->members[j];
            if (m->is_alive && m->agent_id >= 0) {
                DrawTextureEx(texAgent, (Vector2){(float)x, (float)y}, 0.0f, (float)agent_icon_size / (float)texAgent.height, WHITE);
                int tx = x + agent_icon_size + 8;
                int ty = y + 4;
                DrawText(TextFormat("Gang: %d", gn->gang_id), tx, ty, 16, (Color){40,40,120,255}); ty += 18;
                DrawText(TextFormat("Member: %d", m->member_id), tx, ty, 14, (Color){40,40,40,255}); ty += 16;
                DrawText(TextFormat("Knowledge: %.1f", m->knowledge), tx, ty, 14, (Color){40,120,40,255}); ty += 16;
                DrawText(TextFormat("Suspicion: %.1f", m->suspicion), tx, ty, 14, (Color){120,40,40,255}); ty += 16;
                DrawText(TextFormat("Faithfulness: %.1f", m->faithfulness), tx, ty, 14, (Color){40,120,40,255});
                shown++;
                col++;
                if (col >= max_per_row) {
                    col = 0;
                    x = x0;
                    y += agent_icon_size + gap + 40;
                } else {
                    x += 220;
                }
            }
        }
    }
    if (shown == 0) {
        DrawText("No active agents.", (int)r.x + PAD, y0, 16, GRAY);
    }
    EndScissorMode();
    // Draw vertical scrollbar if needed
    if ((float)content_height > view.height) {
        float ratio = view.height / (float)content_height;
        float bw = ratio * view.height;
        float by = view.y + (agentScroll / (float)content_height) * (view.height - bw);
        DrawRectangle((int)(view.x + view.width - 6), (int)by, 4, (int)bw, DARKGRAY);
        // Scroll with mouse wheel if mouse is over agent panel
        if (CheckCollisionPointRec(GetMousePosition(), view)) {
            agentScroll -= GetMouseWheelMove() * 40.0f;
            if (agentScroll < 0) agentScroll = 0;
            if (agentScroll > (float)content_height - view.height) agentScroll = (float)content_height - view.height;
        }
    } else {
        agentScroll = 0.f;
    }
}
static void box_game(Rectangle r,const Game* g,const Config* c){
    panel(r,"Game");
    int y=(int)r.y+40;
    DrawText(TextFormat("Elapsed  : %d s",g->elapsed_time),
             (int)(r.x+PAD),y,18,BLACK); y+=24;
    DrawText(TextFormat("Success  : %d/%d",g->num_successfull_plans,
                        c->max_successful_plans),        (int)(r.x+PAD),y,16,BLACK); y+=18;
    DrawText(TextFormat("Thwarted : %d/%d",g->num_thwarted_plans,
                        c->max_thwarted_plans),          (int)(r.x+PAD),y,16,BLACK); y+=18;
    DrawText(TextFormat("Executed : %d/%d",g->num_executed_agents,
                        c->max_executed_agents),         (int)(r.x+PAD),y,16,BLACK);
}
static void box_police(Rectangle r){ 
    panel(r,"Police");
    // Draw police icon at the top center of the box
    int icon_size = 64;
    int px = (int)(r.x + r.width/2.0f - (float)icon_size/2.0f);
    int py = (int)(r.y + 32);
    DrawTextureEx(texPolice, (Vector2){(float)px, (float)py}, 0.0f, (float)icon_size / (float)texPolice.height, WHITE);
}

static int collect_active(const Game* g,const Config* cfg,int idx[]){
    int n=0;
    for(int i=0;i<cfg->num_gangs;i++){
        const Gang* gn=&g->gangs[i];
        if(gn->num_alive_members>0||gn->plan_in_progress||gn->num_agents>0)
            idx[n++]=i;
    }
    return n;
}

/* gang cards */
static void box_gangs(Rectangle r, const Game* g, const Config* cfg) {
    panel(r, "Gangs");
    hScroll += (float)(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 12.0f;
    if (hScroll < 0) hScroll = 0;
    vScroll -= GetMouseWheelMove() * 40.0f;
    if (vScroll < 0) vScroll = 0;
    int idx[cfg->num_gangs];
    int total = collect_active(g, cfg, idx);
    int gangs_per_row = 2;
    float gap = 24.0f;
    float cardW = CARD_W;
    float startX = r.x + PAD;
    float startY = r.y + 70 - vScroll;
    Rectangle view = {r.x + 1, r.y + 70, r.width - 2, r.height - 71};
    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
    if (total == 0) {
        DrawText("No gangs yet …", (int)(view.x + PAD), (int)(view.y + 20), 22, GRAY);
    } else {
        for (int k = 0; k < total; k++) {
            int gi = idx[k];
            const Gang* gn = &g->gangs[gi];
            int members = gn->num_alive_members;
            int perRow = 4;
            int rowCount = (members + perRow - 1) / perRow;
            float cardH = 170.0f + (float)rowCount * 100.0f;
            int row = k / gangs_per_row;
            int col = k % gangs_per_row;
            float cardX = startX + (float)col * (cardW + gap);
            float cardY = startY + (float)row * (cardH + gap);
            DrawRectangle((int)cardX, (int)cardY, (int)(CARD_W - 12), (int)(cardH - 10), (Color){235, 240, 255, 255});
            DrawRectangleLines((int)cardX, (int)cardY, (int)(CARD_W - 12), (int)(cardH - 10), (Color){120,120,180,255});
            float tx = cardX + 14, ty = cardY + 12;
            DrawText(TextFormat("Gang %d | %s", gi, target_name(gn->target_type)), (int)tx, (int)ty, 20, (Color){30,30,120,255}); ty += 28;
            DrawText(TextFormat("State: %s", gn->plan_in_progress ? "Executing" : (gn->members_ready == gn->num_alive_members && gn->num_alive_members ? "Ready" : "Preparing")), (int)tx, (int)ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("Ready: %d/%d", gn->members_ready, gn->num_alive_members), (int)tx, (int)ty, 14, BLACK); ty += 18;
            DrawText(TextFormat("Success: %d", gn->num_successful_plans), (int)tx, (int)ty, 14, (Color){30,120,30,255}); ty += 18;
            DrawText(TextFormat("Thwart: %d", gn->num_thwarted_plans), (int)tx, (int)ty, 14, (Color){120,30,30,255}); ty += 18;
            DrawText(TextFormat("ExecAg: %d", gn->num_executed_agents), (int)tx, (int)ty, 14, (Color){120,60,30,255}); ty += 18;
            DrawText(TextFormat("Agents: %d", gn->num_agents), (int)tx, (int)ty, 14, (Color){30,30,120,255}); ty += 18;
            DrawText(TextFormat("Notoriety: %.1f", gn->notoriety), (int)tx, (int)ty, 14, (Color){120,30,120,255}); ty += 22;
            // Draw members
            const Member* mem = gn->members;
            // Find highest rank (leader)
            int leader_idx = -1, leader_rank = -9999;
            for (int m = 0; m < members; m++) {
                if (mem[m].is_alive && mem[m].rank > leader_rank) {
                    leader_rank = mem[m].rank;
                    leader_idx = m;
                }
            }
            for (int m = 0; m < members; m++) {
                float mx = tx + 5 + (float)(m % perRow) * 64.0f;
                float my = ty + ((float)m / (float)perRow) * 90.0f;
                if (m == leader_idx) {
                    DrawText("Leader", (int)(mx - 18), (int)(my - 32), 14, (Color){30,30,120,255});
                }
                draw_member(mx, my, &mem[m]);
            }
        }
    }
    EndScissorMode();
    // Optional: draw vertical scrollbar if content overflows
    int rows = (total + gangs_per_row - 1) / gangs_per_row;
    float totalHeight = (float)rows * (170.0f + 100.0f) + (float)(rows-1)*gap;
    if (totalHeight > view.height) {
        float ratio = view.height / totalHeight;
        float bw = ratio * view.height;
        float by = view.y + (vScroll / totalHeight) * (view.height - bw);
        DrawRectangle((int)(view.x + view.width - 6), (int)by, 4, (int)bw, DARKGRAY);
    }
}

/* -----------------------------------------------------------------*/
int main(void)
{
    printf("[DEBUG] Starting graphics viewer main()\n");
    /* load configuration (disk) */
    Config cfg;
    if(load_config(CONFIG_PATH,&cfg)==-1){
        fprintf(stderr,"viewer: cannot load config\n");
        printf("[DEBUG] Failed to load config from %s\n", CONFIG_PATH);
        return 1;
    }
    printf("[DEBUG] Loaded config: max_gangs=%d, max_gang_size=%d\n", cfg.max_gangs, cfg.max_gang_size);

    /* --- open shared memory read-only --- */
    size_t gameSz = sizeof(Game);
    size_t gangsSz = cfg.max_gangs*sizeof(Gang);
    size_t memSz  = cfg.max_gangs*cfg.max_gang_size*sizeof(Member);
    size_t totalSz= gameSz+gangsSz+memSz;
    printf("[DEBUG] Shared memory sizes: gameSz=%zu, gangsSz=%zu, memSz=%zu, totalSz=%zu\n", gameSz, gangsSz, memSz, totalSz);

    int fd = shm_open(GAME_SHM_NAME,O_RDONLY,0);
    if(fd==-1){ perror("viewer: shm_open"); printf("[DEBUG] shm_open failed\n"); return 1; }
    printf("[DEBUG] shm_open succeeded, fd=%d\n", fd);

    void* shm = mmap(NULL,totalSz,PROT_READ,MAP_SHARED,fd,0);
    if(shm==MAP_FAILED){ perror("viewer: mmap"); printf("[DEBUG] mmap failed\n"); return 1; }
    close(fd);
    printf("[DEBUG] mmap succeeded, shm=%p\n", shm);

    const Game* gshm = (const Game*)shm;

    /* allocate snapshot buffer */
    Game* snap = malloc(totalSz);
    if(!snap){ fprintf(stderr,"viewer: malloc snapshot\n"); printf("[DEBUG] malloc snapshot failed\n"); return 1; }
    printf("[DEBUG] malloc snapshot succeeded, snap=%p\n", snap);

    /* init raylib */
    printf("[DEBUG] Calling InitWindow()\n");
    InitWindow(WIN_W,WIN_H,"Bakery Gang Viewer (read-only)");
    printf("[DEBUG] Window initialized\n");
    texGang = mustLoad(ASSETS_PATH "gang.png");
    texAgent = mustLoad(ASSETS_PATH "agent.png");
    texPolice = mustLoad(ASSETS_PATH "police.png");
    printf("[DEBUG] Textures loaded: gang=%d, agent=%d, police=%d\n", texGang.id, texAgent.id, texPolice.id);
    if (texGang.id == 0 || texAgent.id == 0 || texPolice.id == 0) {
        fprintf(stderr, "ERROR: Failed to load assets/gang.png, assets/agent.png, or assets/police.png\n");
        printf("[DEBUG] Texture load failed\n");
        return 1;
    }
    SetTargetFPS(60);
    printf("[DEBUG] Entering main loop\n");

    while(!WindowShouldClose()){
        memcpy(snap,gshm,totalSz);
        fix_snapshot(snap,&cfg);
        __sync_synchronize();
        // Patch: If cfg->num_gangs is 0, try to infer from shared memory (by checking max_member_count)
        int guess = 0;
        for (int i = 0; i < cfg.max_gangs; ++i) {
            if (snap->gangs[i].max_member_count > 0)
                ++guess;
        }
        if (guess > 0) cfg.num_gangs = guess;
        BeginDrawing();
          ClearBackground(COL_BG);
          box_police (R_POL);
          box_agents (R_AGT,snap,&cfg);
          box_game   (R_GME,snap,&cfg);
          box_gangs  (R_GAN,snap,&cfg);
        EndDrawing();
    }
    printf("[DEBUG] Exited main loop\n");
    UnloadTexture(texGang);
    UnloadTexture(texAgent);
    UnloadTexture(texPolice);
    CloseWindow();
    munmap(shm,totalSz);
    free(snap);
    printf("[DEBUG] Clean exit\n");
    return 0;
}