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
#include "police.h"
#include "shared_mem_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "random.h"
#include "semaphores_utils.h"

/*──────────────────────── window/layout ────────────────────────*/
#define WIN_W 1300
#define WIN_H 700
#define SIDE_W 260
#define POL_W  320
#define POL_X  SIDE_W
#define PAD    10
#define COL_BG (Color){245,245,245,255}

#define NEW_SIDE_W 320
#define NEW_POL_W  320
#define NEW_GAN_W  (WIN_W-NEW_POL_W)
#define NEW_GAN_X  (NEW_POL_W)

static Rectangle R_POL = {0, 0,        NEW_POL_W , WIN_H-150};
static Rectangle R_GME = {0, WIN_H-150,    WIN_W, 150};
static Rectangle R_GAN = {NEW_GAN_X, 0,    NEW_GAN_W, WIN_H-150};

/* gang-card constants */
#define CARD_W_UPDATED 300.f     // Updated card width
#define BASE_CARD_H 220.f        /* header + margins for gang info (increased for XP/rank data) */
static float hScroll = 0.f;
static float vScroll = 0.f;

/* Constants for member drawing within a card */
#define MEMBER_ICON_SIZE 36.0f
#define MEMBER_INFO_LINE_HEIGHT 12
#define MEMBER_INFO_FONT_SIZE 10

/* Spacing for members within the card's member grid */
#define MEMBER_GRID_CELL_WIDTH 72.f  // Horizontal space allocated for one member (center to center)
#define MEMBER_GRID_CELL_HEIGHT 140.f // Vertical space allocated for one member (increased for XP/rank data)


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

/*──────────────────────── shared-memory snapshot helpers ───────*/
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

    // Info below icon
    float current_info_y = member_draw_start_y + MEMBER_ICON_SIZE + 8; // More space below icon
    float text_x = member_draw_start_x;

    // Knowledge
    char knowledge_text[16];
    snprintf(knowledge_text, sizeof(knowledge_text), "K: %.0f%%", m->knowledge * 100.0f);
    DrawText(knowledge_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, DARKGRAY);
    current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;

    // XP and Rank
    char xp_text[20];
    snprintf(xp_text, sizeof(xp_text), "XP: %d", m->XP);
    DrawText(xp_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, BLUE);
    current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;
    
    char rank_text[20];
    snprintf(rank_text, sizeof(rank_text), "Rank: %d", m->rank);
    DrawText(rank_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, DARKGREEN);
    current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;

    // Prep Contribution (Text and Progress Bar)
    float prep_pct = m->prep_contribution > 100 ? 100.0f : (float)m->prep_contribution;
    Color prep_bar_color = (prep_pct > 70) ? DARKGREEN : (prep_pct > 30 ? ORANGE : RED);
    char prep_text[16];
    snprintf(prep_text, sizeof(prep_text), "P: %.0f%%", prep_pct);
    DrawText(prep_text, (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, BLACK);
    float prep_text_width_measured = (float)MeasureText("P:100%", MEMBER_INFO_FONT_SIZE); // Cast to float
    float bar_start_x = text_x + prep_text_width_measured + 2;
    float bar_available_width = (member_draw_start_x + MEMBER_ICON_SIZE) - bar_start_x;
    if (bar_available_width < 10) bar_available_width = 10;
    DrawRectangle((int)bar_start_x, (int)current_info_y + 1, (int)bar_available_width, MEMBER_INFO_FONT_SIZE - 2, LIGHTGRAY);
    DrawRectangle((int)bar_start_x, (int)current_info_y + 1, (int)(bar_available_width * prep_pct / 100.f), MEMBER_INFO_FONT_SIZE - 2, prep_bar_color);
    current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;

    // Agent status and data (if applicable)
    if (m->agent_id >= 0) {
        DrawText("AGENT", (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, RED);
        current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;
        DrawText(TextFormat("AgentID: %d", m->agent_id), (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, RED);
        current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;
        DrawText(TextFormat("S: %.2f", m->suspicion), (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, BLACK);
        current_info_y += MEMBER_INFO_LINE_HEIGHT + 2;
        DrawText(TextFormat("F: %.2f", m->faithfulness), (int)text_x, (int)current_info_y, MEMBER_INFO_FONT_SIZE, BLACK);
    }
}


/*──────────────────────── boxes: agents/game/police ────────────*/
static void box_game(Rectangle r,const Config *cfg, ShmPtrs snap){
    const Game *g=snap.shared_game;
    panel(r,"Game Statistics");
    int y=(int)r.y+40;
    DrawText(TextFormat("Elapsed Time: %d seconds",g->elapsed_time),
             (int)(r.x+PAD),y,16,BLACK); y+=20;
    
    // Gang vs Police Statistics
    DrawText("Gang Operations:",(int)(r.x+PAD),y,14,DARKBLUE); y+=16;
    DrawText(TextFormat("  Successful: %d/%d",g->num_successfull_plans,
                        cfg->max_successful_plans),
             (int)(r.x+PAD+10),y,14,DARKGREEN); y+=16;
    DrawText(TextFormat("  Thwarted: %d/%d",g->num_thwarted_plans,
                        cfg->max_thwarted_plans),
             (int)(r.x+PAD+10),y,14,RED); y+=16;
             
    // Police Effectiveness
    DrawText("Police Operations:",(int)(r.x+PAD),y,14,DARKBLUE); y+=16;
    int total_plans = g->num_successfull_plans + g->num_thwarted_plans;
    float police_success_rate = (total_plans > 0) ? ((float)g->num_thwarted_plans / total_plans * 100.0f) : 0.0f;
    Color success_color = (police_success_rate > 70) ? DARKGREEN : (police_success_rate > 40) ? ORANGE : RED;
    DrawText(TextFormat("  Success Rate: %.1f%%", police_success_rate),
             (int)(r.x+PAD+10),y,14,success_color); y+=16;
    DrawText(TextFormat("  Agents Lost: %d/%d",g->num_executed_agents,
                        cfg->max_executed_agents),
             (int)(r.x+PAD+10),y,14,RED); y+=16;
             
    // Game Progress
    y += 8;
    DrawText("Game Progress:",(int)(r.x+PAD),y,14,DARKBLUE); y+=16;
    
    // Progress bars for limits
    float success_pct = (cfg->max_successful_plans > 0) ? 
                       ((float)g->num_successfull_plans / cfg->max_successful_plans * 100.0f) : 0.0f;
    float thwart_pct = (cfg->max_thwarted_plans > 0) ? 
                      ((float)g->num_thwarted_plans / cfg->max_thwarted_plans * 100.0f) : 0.0f;
    float agent_pct = (cfg->max_executed_agents > 0) ? 
                     ((float)g->num_executed_agents / cfg->max_executed_agents * 100.0f) : 0.0f;
    
    int bar_width = 120;
    int bar_height = 10;
    
    // Success progress bar
    DrawText("Gang Wins:", (int)(r.x+PAD+10), y, 12, BLACK);
    DrawRectangle((int)(r.x+PAD+80), y+2, bar_width, bar_height, LIGHTGRAY);
    DrawRectangle((int)(r.x+PAD+80), y+2, (int)(bar_width * success_pct / 100.0f), bar_height, GREEN);
    y += 16;
    
    // Thwart progress bar
    DrawText("Police Wins:", (int)(r.x+PAD+10), y, 12, BLACK);
    DrawRectangle((int)(r.x+PAD+80), y+2, bar_width, bar_height, LIGHTGRAY);
    DrawRectangle((int)(r.x+PAD+80), y+2, (int)(bar_width * thwart_pct / 100.0f), bar_height, BLUE);
    y += 16;
    
    // Agent loss progress bar
    DrawText("Agent Loss:", (int)(r.x+PAD+10), y, 12, BLACK);
    DrawRectangle((int)(r.x+PAD+80), y+2, bar_width, bar_height, LIGHTGRAY);
    DrawRectangle((int)(r.x+PAD+80), y+2, (int)(bar_width * agent_pct / 100.0f), bar_height, RED);
}
// ──────────────── POLICE BOX ────────────────
static void box_police(Rectangle r, ShmPtrs snap)
{
    panel(r,"Police");
    PoliceForce *pf = NULL;
    const Game *g = snap.shared_game;
    if (!g) {
        DrawText("No Game struct!",(int)r.x+PAD,(int)r.y+40,18,RED);
        return;
    }
    pf = &g->police_force;
    int icon = 32; // Smaller icon for more space
    int y = (int)r.y + 40;
    
    // Police Statistics
    DrawText("Police Statistics:",(int)r.x+PAD, y, 16, DARKBLUE); y += 20;
    DrawText(TextFormat("Plans Thwarted: %d", g->num_thwarted_plans), (int)r.x+PAD+10, y, 14, DARKGREEN); y += 16;
    DrawText(TextFormat("Agents Executed: %d", g->num_executed_agents), (int)r.x+PAD+10, y, 14, RED); y += 16;
    
    // Calculate total active agents
    int total_active_agents = 0;
    for (int i = 0; i < pf->num_officers; ++i) {
        total_active_agents += pf->officers[i].num_agents;
    }
    DrawText(TextFormat("Active Agents: %d", total_active_agents), (int)r.x+PAD+10, y, 14, BLUE); y += 18;
    
    // Arrested Gangs
    DrawText("Arrested Gangs:",(int)r.x+PAD, y, 16, DARKBLUE); y += 18;
    bool has_arrests = false;
    for (int i = 0; i < MAX_GANGS_POLICE; ++i) {
        if (pf->arrested_gangs[i] > 0) {
            Color arrest_color = (pf->arrested_gangs[i] > 10) ? RED : ORANGE;
            DrawText(TextFormat("Gang %d: %ds (Officer %d)", i, pf->arrested_gangs[i], i), 
                     (int)r.x+PAD+10, y, 14, arrest_color); 
            y += 15;
            has_arrests = true;
        }
    }
    if (!has_arrests) {
        DrawText("None", (int)r.x+PAD+10, y, 14, GRAY); y += 15;
    }
    y += 6;
    
    // Officers
    DrawText("Officers:",(int)r.x+PAD, y, 16, DARKBLUE); y += 18;
    for (int i = 0; i < pf->num_officers && y < (int)(r.y + r.height - 80); ++i) {
        PoliceOfficer *po = &pf->officers[i];
        Color c = po->is_active ? DARKGREEN : GRAY;
        
        // Draw smaller police icon
        DrawTextureEx(texPolice, (Vector2){r.x+PAD, (float)y}, 0.0f, (float)icon/(float)texPolice.width, WHITE);
        float text_x = r.x+PAD+(float)icon+6.0f;
        
        // Officer basic info
        DrawText(TextFormat("ID:%d Gang:%d", po->police_id, po->gang_id_monitoring), (int)text_x, y, 13, c); y += 14;
        
        // Knowledge with color coding
        Color knowledge_color = (po->knowledge_level > 0.8f) ? DARKGREEN : 
                               (po->knowledge_level > 0.5f) ? ORANGE : RED;
        DrawText(TextFormat("Knowledge: %.2f", po->knowledge_level), (int)text_x, y, 12, knowledge_color); y += 13;
        
        // Agent information
        if (po->num_agents > 0) {
            DrawText(TextFormat("Agents: %d", po->num_agents), (int)text_x, y, 12, BLUE); y += 13;
            
            // Show active agent knowledge levels
            for (int j = 0; j < po->num_agents && j < 3; ++j) { // Show max 3 agents to save space
                if (po->agents[j].is_active) {
                    Color agent_color = (po->agents[j].knowledge_level > 0.8f) ? DARKGREEN : 
                                       (po->agents[j].knowledge_level > 0.5f) ? ORANGE : RED;
                    DrawText(TextFormat("  Agent%d: %.2f", po->agents[j].agent_id, po->agents[j].knowledge_level), 
                            (int)text_x, y, 11, agent_color); 
                    y += 12;
                }
            }
            if (po->num_agents > 3) {
                DrawText(TextFormat("  +%d more...", po->num_agents - 3), (int)text_x, y, 11, GRAY); y += 12;
            }
        } else {
            DrawText("No Agents", (int)text_x, y, 12, GRAY); y += 13;
        }
        
        // Gang arrest status
        if (pf->arrested_gangs[po->gang_id_monitoring] > 0) {
            DrawText("GANG ARRESTED", (int)text_x, y, 11, RED); y += 12;
        }
        
        y += 6; // Extra spacing between officers
    }
}

/*──────────────────────── gangs panel ──────────────────────────*/
static int collect_active(const Config*cfg,int *out, ShmPtrs snap){
    int n=0;
    for(int i=0;i<cfg->num_gangs;i++){
        const Gang* g=&snap.gangs[i];
        if(g->num_alive_members > 0 || g->plan_in_progress || g->num_agents > 0) out[n++]=i;
    }
    return n;
}

static void box_gangs(Rectangle r,const Config *cfg, ShmPtrs snap){
    panel(r,"Gangs");

    hScroll+= (float)(IsKeyDown(KEY_RIGHT)-IsKeyDown(KEY_LEFT))*12.f;
    if(hScroll<0) hScroll=0;
    // Ensure hScroll doesn't scroll too far right (content width needs to be known)
    // This part is omitted for brevity but would be needed for robust horizontal scrolling limits.

    vScroll-= GetMouseWheelMove()*40.f;
    if(vScroll<0) vScroll=0;
    // Vertical scroll limit also needs total content height, calculated later.

    int idx[cfg->num_gangs];
    int total_active_gangs = collect_active(cfg,idx, snap);

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
        float card_actual_height = BASE_CARD_H + ((float)num_member_rows_on_card * MEMBER_GRID_CELL_HEIGHT); // Cast to float

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


        float current_card_x = (float)(r.x + PAD) + (float)panel_col_idx * (CARD_W_UPDATED + card_gap_x) - hScroll; // Cast to float
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
        Color gang_state_color = BLACK;
        
        // Check if gang is arrested
        if (snap.shared_game && snap.shared_game->police_force.arrested_gangs[gang_array_idx] > 0) {
            gang_state_text = "ARRESTED";
            gang_state_color = RED;
        } else if (current_gang->plan_in_progress) {
            gang_state_text = "Executing";
            gang_state_color = ORANGE;
        } else if (alive_members_count > 0 && current_gang->members_ready == alive_members_count) {
            gang_state_text = "Ready";
            gang_state_color = DARKGREEN;
        } else if (alive_members_count == 0 && !current_gang->plan_in_progress) {
            gang_state_text = "Preparing"; // Or "Idle"
            gang_state_color = BLACK;
        }

        DrawText(TextFormat("State  : %s",gang_state_text),(int)text_start_x_in_card,(int)text_start_y_in_card,14,gang_state_color); text_start_y_in_card+=18;
        
        // Show arrest time if gang is arrested
        if (snap.shared_game && snap.shared_game->police_force.arrested_gangs[gang_array_idx] > 0) {
            DrawText(TextFormat("Release: %ds",snap.shared_game->police_force.arrested_gangs[gang_array_idx]),
                     (int)text_start_x_in_card,(int)text_start_y_in_card,14,RED); text_start_y_in_card+=18;
        }
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
                float member_center_x = text_start_x_in_card + (drawn_member_visual_count % members_per_row_on_card) * MEMBER_GRID_CELL_WIDTH + (MEMBER_GRID_CELL_WIDTH / 2.0f) - (PAD/2.0f);
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
    ShmPtrs snap;          /* rebuilt every frame */

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
    while(!WindowShouldClose()){
        BeginDrawing();
          ClearBackground(COL_BG);
          box_police(R_POL, snap);
          box_game  (R_GME,&cfg, snap);
          box_gangs (R_GAN,&cfg, snap);
        EndDrawing();
    }
    UnloadTexture(texGang);
    UnloadTexture(texAgent);
    UnloadTexture(texPolice);
    CloseWindow();
    return 0;
}
