/********************************************************************
 * graphics.c  –  viewer for the “Bakery/OCF” simulation (2025-05-12)
 *   ▸ reads shared-memory segment read-only (no semaphores needed)
 *   ▸ copies whole block each frame and rebuilds pointers
 *   ▸ renders with raylib 5.x
 *******************************************************************/
#include "raylib.h"
#include "config.h"
#include "game.h"
#include "gang.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/*──────────────────────── window/layout ────────────────────────*/
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

/* gang-card constants */
#define CARD_W_UPDATED 300.f     // Updated card width
#define BASE_CARD_H 170.f        /* header + margins for gang info */
static float hScroll = 0.f;
static float vScroll = 0.f;

/* Constants for member drawing within a card */
#define MEMBER_ICON_SIZE 36.0f
#define MEMBER_INFO_LINE_HEIGHT 12
#define MEMBER_INFO_FONT_SIZE 10

/* Spacing for members within the card's member grid */
#define MEMBER_GRID_CELL_WIDTH 72.f  // Horizontal space allocated for one member (center to center)
#define MEMBER_GRID_CELL_HEIGHT 95.f // Vertical space allocated for one member (center to center)


/*──────────────────────── assets ───────────────────────────────*/
static Texture2D texGang, texAgent, texPolice;

static Texture2D mustLoad(const char *path){
    Texture2D t = LoadTexture(path);
    if(!t.id){
        TraceLog(LOG_FATAL,"cannot load %s (cwd=%s)",path,getcwd(NULL,0));
        exit(1);
    }
    return t;
}
static int file_exists(const char *p){ struct stat st; return !stat(p,&st); }

/*──────────────────────── shared-memory snapshot helpers ───────*/
static ShmPtrs snap;          /* rebuilt every frame */
Game *shared_game;

/*──────────────────────── tiny helpers ─────────────────────────*/
static const char *target_name(TargetType t){
    static const char* n[]={"Bank","Jewelry","Drugs","Art",
                            "Kidnap","Blackmail","Arms"};
    return (t<NUM_TARGETS)? n[t] : "U";
}
static void panel(Rectangle r,const char *title){
    DrawRectangleLinesEx(r,2,GRAY);
    DrawText(title,(int)(r.x+PAD),(int)(r.y+PAD),20,DARKBLUE);
}

/*──────────────────────── member drawing ───────────────────────*/
// cx, cy are the CENTER coordinates for the member's drawing area
static void draw_member(float cx, float cy, const Member *m) {
    float icon_half_size = MEMBER_ICON_SIZE / 2.0f;
    float member_draw_start_x = cx - icon_half_size;
    float member_draw_start_y = cy - icon_half_size;

    // Icon: Use texAgent if member is an agent, otherwise texGang
    Texture2D current_member_tex = (m->agent_id >= 0) ? texAgent : texGang;
    DrawTextureEx(current_member_tex, (Vector2){member_draw_start_x, member_draw_start_y}, 0.0f, MEMBER_ICON_SIZE / current_member_tex.width, WHITE);

    // Rank (on icon, top-right of icon space)
    char rank_text[8];
    // Info below icon
    float current_info_y = member_draw_start_y + MEMBER_ICON_SIZE + 4; // Start Y for text below icon
    float text_x = member_draw_start_x;

    // Knowledge
    char knowledge_text[16];
    snprintf(knowledge_text, sizeof(knowledge_text), "K: %.0f%%", m->knowledge * 100.0f);
    DrawText(knowledge_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, DARKGRAY);
    current_info_y += MEMBER_INFO_LINE_HEIGHT;

    // Prep Contribution (Text and Progress Bar)
    float prep_pct = m->prep_contribution > 100 ? 100.0f : (float)m->prep_contribution;
    Color prep_bar_color = (prep_pct > 70) ? DARKGREEN : (prep_pct > 30 ? ORANGE : RED);
    
    char prep_text[16];
    snprintf(prep_text, sizeof(prep_text), "P: %.0f%%", prep_pct);
    DrawText(prep_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, BLACK);
    
    // Draw bar next to "P: XX%" text, aligned to the right edge of the icon space
    float prep_text_width_measured = MeasureText("P:100%", MEMBER_INFO_FONT_SIZE); // Measure widest possible prep text
    float bar_start_x = text_x + prep_text_width_measured + 2;
    float bar_available_width = (member_draw_start_x + MEMBER_ICON_SIZE) - bar_start_x;
    if (bar_available_width < 10) bar_available_width = 10; // min width for visibility
    
    DrawRectangle((int)bar_start_x, (int)current_info_y + 1, (int)bar_available_width, MEMBER_INFO_FONT_SIZE - 2, LIGHTGRAY);
    DrawRectangle((int)bar_start_x, (int)current_info_y + 1, (int)(bar_available_width * prep_pct / 100.f), MEMBER_INFO_FONT_SIZE - 2, prep_bar_color);
    current_info_y += MEMBER_INFO_LINE_HEIGHT;

    // Agent status text (if applicable)
    if (m->agent_id >= 0) {
        DrawText("AGENT", (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, RED);
        // current_info_y += MEMBER_INFO_LINE_HEIGHT; // If more info were to follow
    }
}


/*──────────────────────── boxes: agents/game/police ────────────*/
static void box_game(Rectangle r,const Config *cfg){
    const Game *g=snap.shared_game;
    panel(r,"Game");
    int y=(int)r.y+40;
    DrawText(TextFormat("Elapsed  : %d s",g->elapsed_time),
             (int)(r.x+PAD),y,18,BLACK); y+=24;
    DrawText(TextFormat("Success  : %d/%d",g->num_successfull_plans,
                        cfg->max_successful_plans),
             (int)(r.x+PAD),y,16,BLACK); y+=18;
    DrawText(TextFormat("Thwarted : %d/%d",g->num_thwarted_plans,
                        cfg->max_thwarted_plans),
             (int)(r.x+PAD),y,16,BLACK); y+=18;
    DrawText(TextFormat("Executed : %d/%d",g->num_executed_agents,
                        cfg->max_executed_agents),
             (int)(r.x+PAD),y,16,BLACK);
}
static void box_police(Rectangle r){
    panel(r,"Police");
    int sz=64;
    int px=(int)(r.x+r.width/2-sz/2), py=(int)r.y+32;
    DrawTextureEx(texPolice,(Vector2){(float)px,(float)py},0.0f,(float)sz/texPolice.width,WHITE);
}
static void box_agents(Rectangle r,const Config *cfg){
    panel(r,"Agents");
    int active=0;
    for(int g_idx=0; g_idx < cfg->num_gangs; g_idx++) {
        for(int m=0; m < snap.gangs[g_idx].max_member_count; m++) {
                if(snap.gang_members[g_idx][m].is_alive &&
                   snap.gang_members[g_idx][m].agent_id >= 0) active++;
        }
    }
    int y=(int)r.y+40;
    DrawText(TextFormat("Active  : %d",active),
             (int)(r.x+PAD),y,18,BLACK); y+=22;
    DrawText(TextFormat("Executed: %d",snap.shared_game->num_executed_agents),
             (int)(r.x+PAD),y,18,BLACK);
}

/*──────────────────────── gangs panel ──────────────────────────*/
static int collect_active(const Config*cfg,int *out){
    int n=0;
    for(int i=0;i<cfg->num_gangs;i++){
        const Gang* g=&snap.gangs[i];
        if(g->num_alive_members > 0 || g->plan_in_progress || g->num_agents > 0) out[n++]=i;
    }
    return n;
}

static void box_gangs(Rectangle r,const Config*cfg){
    panel(r,"Gangs");

    hScroll+= (float)(IsKeyDown(KEY_RIGHT)-IsKeyDown(KEY_LEFT))*12.f;
    if(hScroll<0) hScroll=0;
    // Ensure hScroll doesn't scroll too far right (content width needs to be known)
    // This part is omitted for brevity but would be needed for robust horizontal scrolling limits.

    vScroll-= GetMouseWheelMove()*40.f;
    if(vScroll<0) vScroll=0;
    // Vertical scroll limit also needs total content height, calculated later.

    int idx[128]; // Assuming max 128 gangs
    int total_active_gangs = collect_active(cfg,idx);

    int members_per_row_on_card = 4;
    int gangs_per_row_in_panel = 2;
    float card_gap_x = 24.f, card_gap_y = 24.f;

    Rectangle view_scissor_area ={r.x+1,r.y+70,r.width-2,r.height-71};
    BeginScissorMode((int)view_scissor_area.x,(int)view_scissor_area.y,(int)view_scissor_area.width,(int)view_scissor_area.height);

    float current_panel_row_y_start = r.y + 70.0f - vScroll; // Start Y for the first row of cards in the panel
    float max_height_this_panel_row = 0.0f;
    float total_content_height = 0; // For vertical scrollbar limit

    for(int k=0; k < total_active_gangs; k++){
        int gang_array_idx = idx[k];
        const Gang* current_gang = &snap.gangs[gang_array_idx];
        int alive_members_count = current_gang->num_alive_members;
        
        int num_member_rows_on_card = (alive_members_count + members_per_row_on_card - 1) / members_per_row_on_card;
        float card_actual_height = BASE_CARD_H + (num_member_rows_on_card * MEMBER_GRID_CELL_HEIGHT);

        int panel_col_idx = k % gangs_per_row_in_panel;

        if (panel_col_idx == 0 && k > 0) { // Starting a new row in the panel
            current_panel_row_y_start += max_height_this_panel_row + card_gap_y;
            total_content_height = current_panel_row_y_start - (r.y + 70.0f - vScroll); // Update content height based on new row start
            max_height_this_panel_row = 0.0f; // Reset for the new row
        }

        max_height_this_panel_row = (card_actual_height > max_height_this_panel_row) ? card_actual_height : max_height_this_panel_row;
        if (k == total_active_gangs -1 || (k+1) % gangs_per_row_in_panel == 0) { // Last card overall or last card in a row
             if (panel_col_idx == 0 && k == 0) { // First card, first row
                total_content_height = max_height_this_panel_row;
            } else if ((k+1) % gangs_per_row_in_panel == 0) { // End of a full row
                // total_content_height is already updated when a new row starts.
                // For the very last row (which might not be full), update based on its max height.
            }
             if (k == total_active_gangs - 1 && total_active_gangs > 0) { // If it's the very last card
                 // Ensure total_content_height reflects the end of this last card's row
                 // If this is the first row (k < gangs_per_row_in_panel)
                 if (total_active_gangs <= gangs_per_row_in_panel) {
                     total_content_height = max_height_this_panel_row;
                 } else {
                     // If it's not the first row, total_content_height was updated at the start of this row.
                     // We need to add the height of this current row.
                     // This logic is getting complex. A simpler way:
                     // total_content_height = (current_panel_row_y_start + max_height_this_panel_row) - (r.y + 70.0f - vScroll);
                     // This should be calculated after the loop for the final content height.
                 }
             }
        }


        float current_card_x = (r.x + PAD) + panel_col_idx * (CARD_W_UPDATED + card_gap_x) - hScroll;
        float current_card_y = current_panel_row_y_start;
                                                                                            
        // Culling: Skip drawing if card is entirely out of view
        if(current_card_y + card_actual_height < view_scissor_area.y) continue; 
        if(current_card_y > view_scissor_area.y + view_scissor_area.height) {
            // If this card is already below the viewport, subsequent cards in this column or row will also be.
            // If we are laying out row by row, we can break.
            // If it's the first card in a new row that's below, then all subsequent rows are also below.
            if (panel_col_idx == 0) break; 
            // else, other cards in this row might still be visible if horizontal scrolling is wide.
            // However, with vertical scrolling, if the top of the row is below, all of it is.
            // So, `break` is likely fine if `current_panel_row_y_start` itself is already too low.
        }


        DrawRectangle((int)current_card_x,(int)current_card_y,(int)(CARD_W_UPDATED-12),(int)(card_actual_height-10),
                      (Color){235,240,255,255});
        DrawRectangleLines((int)current_card_x,(int)current_card_y,(int)(CARD_W_UPDATED-12),(int)(card_actual_height-10),
                           (Color){120,120,180,255});

        float text_start_x_in_card = current_card_x + 14;
        float text_start_y_in_card = current_card_y + 12;
        DrawText(TextFormat("Gang %d | %s",gang_array_idx,target_name(current_gang->target_type)),
                 (int)text_start_x_in_card,(int)text_start_y_in_card,20,(Color){30,30,120,255}); text_start_y_in_card+=28;

        const char* gang_state_text = "Preparing";
        if (current_gang->plan_in_progress) {
            gang_state_text = "Executing";
        } else if (alive_members_count > 0 && current_gang->members_ready == alive_members_count) {
            gang_state_text = "Ready";
        } else if (alive_members_count == 0 && !current_gang->plan_in_progress) {
            gang_state_text = "Preparing"; // Or "Idle"
        }

        DrawText(TextFormat("State  : %s",gang_state_text),(int)text_start_x_in_card,(int)text_start_y_in_card,14,BLACK); text_start_y_in_card+=18;
        DrawText(TextFormat("Ready  : %d/%d",current_gang->members_ready, alive_members_count),
                 (int)text_start_x_in_card,(int)text_start_y_in_card,14,BLACK); text_start_y_in_card+=18;
        DrawText(TextFormat("Success: %d",current_gang->num_successful_plans),
                 (int)text_start_x_in_card,(int)text_start_y_in_card,14,(Color){30,120,30,255}); text_start_y_in_card+=18;
        DrawText(TextFormat("Thwart : %d",current_gang->num_thwarted_plans),
                 (int)text_start_x_in_card,(int)text_start_y_in_card,14,(Color){120,30,30,255}); text_start_y_in_card+=18;
        DrawText(TextFormat("Agents : %d",current_gang->num_agents),
                 (int)text_start_x_in_card,(int)text_start_y_in_card,14,(Color){30,30,120,255}); text_start_y_in_card+=22; 

        /* draw members */
        const Member *member_array_for_gang = snap.gang_members[gang_array_idx];
        int leader_member_idx = -1;
        int highest_rank_found = -999;
        for (int m_scan = 0; m_scan < current_gang->max_member_count; m_scan++) {
            if (member_array_for_gang[m_scan].is_alive && member_array_for_gang[m_scan].rank > highest_rank_found) {
                highest_rank_found = member_array_for_gang[m_scan].rank;
                leader_member_idx = m_scan;
            }
        }

        float member_grid_y_offset_in_card = text_start_y_in_card; 
        int drawn_member_visual_count = 0;
        for (int m_loop_idx = 0; m_loop_idx < current_gang->max_member_count; m_loop_idx++) {
            if (member_array_for_gang[m_loop_idx].is_alive) {
                // Calculate center for the member's drawing area
                float member_center_x = text_start_x_in_card + (drawn_member_visual_count % members_per_row_on_card) * MEMBER_GRID_CELL_WIDTH + (MEMBER_GRID_CELL_WIDTH / 2.0f) - (PAD/2.0f); // Adjust X to be centered in cell
                float member_center_y = member_grid_y_offset_in_card + (drawn_member_visual_count / members_per_row_on_card) * MEMBER_GRID_CELL_HEIGHT + (MEMBER_GRID_CELL_HEIGHT / 2.0f);
                
                if (m_loop_idx == leader_member_idx) {
                    char leader_label[] = "Leader";
                    int leader_text_width = MeasureText(leader_label, 12);
                    DrawText(leader_label, (int)(member_center_x - (MEMBER_ICON_SIZE/2.0f) - leader_text_width - 2) , (int)(member_center_y - 6), 12, (Color){0,0,200,255});
                }
                draw_member(member_center_x, member_center_y, &member_array_for_gang[m_loop_idx]);
                drawn_member_visual_count++;
            }
        }
    }
    // After the loop, calculate the final total_content_height for the scrollbar
    if (total_active_gangs > 0) {
        total_content_height = (current_panel_row_y_start + max_height_this_panel_row) - (r.y + 70.0f - vScroll) + vScroll;
    } else {
        total_content_height = 0;
    }
    
    // Vertical scrollbar logic (optional, if content overflows)
    if (total_content_height > view_scissor_area.height) {
        float scrollbar_track_height = view_scissor_area.height;
        float scrollbar_thumb_height = (view_scissor_area.height / total_content_height) * scrollbar_track_height;
        if (scrollbar_thumb_height < 20) scrollbar_thumb_height = 20; // Min thumb height

        float scrollbar_y_pos = view_scissor_area.y + (vScroll / (total_content_height - view_scissor_area.height)) * (scrollbar_track_height - scrollbar_thumb_height);
        if (vScroll == 0) scrollbar_y_pos = view_scissor_area.y;
        if (vScroll >= total_content_height - view_scissor_area.height) scrollbar_y_pos = view_scissor_area.y + scrollbar_track_height - scrollbar_thumb_height;


        DrawRectangle((int)(view_scissor_area.x + view_scissor_area.width - 8), (int)view_scissor_area.y, 6, (int)scrollbar_track_height, LIGHTGRAY);
        DrawRectangle((int)(view_scissor_area.x + view_scissor_area.width - 8), (int)scrollbar_y_pos, 6, (int)scrollbar_thumb_height, DARKGRAY);
        
        // Clamp vScroll
        if (vScroll > total_content_height - view_scissor_area.height && total_content_height > view_scissor_area.height) {
            vScroll = total_content_height - view_scissor_area.height;
        }
    } else {
        vScroll = 0; // No scroll needed if content fits
    }


    EndScissorMode();
}

/*───────────────────────── main ───────────────────────────────*/
int main(int argc, char *argv[]){
    Config cfg;

    if (argc < 2) {
        fprintf(stderr, "graphics viewer: Missing serialized config argument\n");
        return 1;
    }
    deserialize_config(argv[1], &cfg);

    shared_game = setup_shared_memory_user(&cfg, &snap);

    InitWindow(WIN_W,WIN_H,"Bakery Gang Viewer (read-only)");
    texGang   = mustLoad(ASSETS_PATH"gang.png");
    texAgent  = mustLoad(ASSETS_PATH"agent.png");
    texPolice = mustLoad(ASSETS_PATH"police.png");
    SetTargetFPS(60);

    // Game *snapshot_buffer=malloc(total_shm_size);
    // if (!snapshot_buffer) {
    //     perror("malloc snapshot_buffer");
    //     munmap(shm_ptr, total_shm_size);
    //     CloseWindow();
    //     return 1;
    // }

    // // Initialize semaphores for synchronization
    // if (init_semaphores() != 0) {
    //     fprintf(stderr, "Graphics: Failed to initialize semaphores\n");
    //     munmap(shm_ptr, total_shm_size);
    //     CloseWindow();
    //     return 1;
    // }

    while(!WindowShouldClose()){


        BeginDrawing();
          ClearBackground(COL_BG);
          box_police(R_POL);
          box_agents(R_AGT,&cfg);
          box_game  (R_GME,&cfg);
          box_gangs (R_GAN,&cfg);
        EndDrawing();
    }

    UnloadTexture(texGang);
    UnloadTexture(texAgent);
    UnloadTexture(texPolice);
    CloseWindow();
    return 0;
}
