
#include "raylib.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "game.h"
#include "gang.h"          /* للحصول على تعريف Gang / Member */

/* ---------- نافذة & ألوان ------------------------------------ */
#define WIN_W 1280
#define WIN_H 720
#define PAD   12
#define COL_BG  (Color){245,245,245,255}

/* ---------- تقسيم الشاشة لأربعة مربعات ------------------------ */
static Rectangle R_POL = {0,        0,        WIN_W/2, WIN_H/2};
static Rectangle R_GAN = {WIN_W/2,  0,        WIN_W/2, WIN_H/2};
static Rectangle R_AGT = {0,        WIN_H/2,  WIN_W/2, WIN_H/2};
static Rectangle R_GME = {WIN_W/2,  WIN_H/2,  WIN_W/2, WIN_H/2};

/* ---------- helpers ------------------------------------------- */
static void draw_panel(Rectangle r,const char *t){
    DrawRectangleLinesEx(r,2,GRAY);
    DrawText(t,(int)r.x+PAD,(int)r.y+PAD,20,DARKBLUE);
}

/* أعلى رتبة داخل العصابة (للعرض فقط) */
static int top_rank(const Gang *gn){
    int top=0;
    for(int i=0;i<gn->members_count;i++)
        if(gn->members[i].rank > top) top = gn->members[i].rank;
    return top;
}

/* ---------- مربعات العرض -------------------------------------- */
static void box_police(Rectangle r,const Game *g,const Config *c){
    draw_panel(r,"Police");
    int y=r.y+40;
    DrawText(TextFormat("Thwarted : %d / %d",g->num_thwarted_plans,
                        c->max_thwarted_plans),(int)r.x+PAD,y,18,BLACK); y+=22;
    DrawText(TextFormat("Successful: %d / %d",g->num_successfull_plans,
                        c->max_successful_plans),(int)r.x+PAD,y,18,BLACK); y+=22;
    DrawText(TextFormat("Executed  : %d / %d",g->num_executed_agents,
                        c->max_executed_agents),(int)r.x+PAD,y,18,BLACK);
}

static void box_gangs(Rectangle r,const Game *g){
    draw_panel(r,"Gangs");
    int y=r.y+40;
    DrawText(TextFormat("Total gangs: %d",g->total_gangs),
             (int)r.x+PAD,y,18,BLACK); y+=24;
    for(int i=0;i<g->total_gangs && y<r.y+r.height-18;i++){
        const Gang *gn=&g->gangs[i];
        DrawText(TextFormat("#%d size=%d agents=%d top-rank=%d",i,
                            gn->members_count, gn->num_agents, top_rank(gn)),
                 (int)r.x+PAD,y,16,BLACK);
        y+=18;
    }
}

static void box_agents(Rectangle r,const Game *g){
    draw_panel(r,"Agents");
    int total=0;
    for(int i=0;i<g->total_gangs;i++) total += g->gangs[i].num_agents;
    int y=r.y+40;
    DrawText(TextFormat("Active agents : %d",total),
             (int)r.x+PAD,y,18,BLACK); y+=22;
    DrawText(TextFormat("Executed      : %d",g->num_executed_agents),
             (int)r.x+PAD,y,18,BLACK);
}

static void box_game(Rectangle r,const Game *g,const Config *c){
    draw_panel(r,"Game");
    int y=r.y+40;
    DrawText(TextFormat("Elapsed time: %d s",g->elapsed_time),
             (int)r.x+PAD,y,18,BLACK); y+=22;
    bool over=(g->num_thwarted_plans   >= c->max_thwarted_plans)  ||
              (g->num_successfull_plans>= c->max_successful_plans)||
              (g->num_executed_agents  >= c->max_executed_agents);
    DrawText(over? "SIMULATION ENDED" : "Running…",
             (int)r.x+PAD,y,over?20:18,over?RED:GREEN);
}

/* ============================================================= */
int main(int argc,char **argv)
{
    if(argc<3){
        fprintf(stderr,"usage: %s <config.txt> /game_shm\n",argv[0]);
        return 1;
    }

    /* --- قراءة ملف الإعدادات عبر الدالة الرسمية فى config.c --- */
    Config cfg={0};
    if(load_config(argv[1], &cfg) != 0){
        perror("config"); return 1;
    }

    /* --- ربط الذاكرة المشتركة للعبة (read-only) ---------------- */
    int fd=shm_open(argv[2],O_RDONLY,0);
    if(fd==-1){ perror("shm_open"); return 1; }
    Game *g=mmap(NULL,sizeof(Game),PROT_READ,MAP_SHARED,fd,0);
    if(g==MAP_FAILED){ perror("mmap"); return 1; }

    /* --- نافذة Raylib ----------------------------------------- */
    InitWindow(WIN_W,WIN_H,"Organized Crime – Viewer");
    SetTargetFPS(30);

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(COL_BG);

        box_police(R_POL,g,&cfg);
        box_gangs (R_GAN,g);
        box_agents(R_AGT,g);
        box_game  (R_GME,g,&cfg);

        EndDrawing();
    }

    /* --- تنظيف ------------------------------------------------ */
    munmap(g,sizeof(Game)); close(fd); CloseWindow();
    return 0;
}
