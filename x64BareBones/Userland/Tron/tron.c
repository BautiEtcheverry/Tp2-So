#include <stdint.h>
#include <stddef.h>
#include "../include/libc.h"
#include "../include/counter.h"
#include "../include/text.h"
#include "../include/actualTime.h" 

#define TRON_W 80
#define TRON_H 45

// Steps limit if frame is late
#define MAX_CATCHUP_STEPS 3
#define TARGET_PX_PER_SEC 300
#define FPS_UPDATE_INTERVAL 4 // Actualizar display cada 30 frames

typedef struct {
    int score;
} player_score_t;

typedef struct {
    uint16_t freq;
    uint16_t duration_ms;
} tone_t;

static const tone_t death_jingle_sequence[] = {
    {440, 200},  // A4
    {0, 100},    // Silence
    {392, 200},  // G4
    {0, 100},    // Silence
    {349, 200},  // F4
    {0, 100},    // Silence
    {330, 400}, // E4 
};

static struct {
    int active;
    size_t index;
    long long next_change_ns;
} death_jingle_state = {0, 0, 0};

typedef enum{
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} dir_t;

static inline dir_t left_of(dir_t d)  { return (dir_t)((d + 3) & 3); }
static inline dir_t right_of(dir_t d) { return (dir_t)((d + 1) & 3); }

/*----------HEADERS----------*/
static void pause_ticks(int t);
static int tron_single_impl(void);
static int tron_multi_impl(void);
static void draw_game_arena(int scr_w, int scr_h, int offset_x, int offset_y, int cell, int grid_w_px, int grid_h_px);
static void draw_trail_and_head(int offset_x, int offset_y, int cell, int old_x, int old_y, int new_x, int new_y, uint32_t color, int  headFlag);
static void draw_fps_overlay(int scr_w, int top_margin, unsigned fps_value);
static void show_game_over(int result, player_score_t *p1_score, player_score_t *p2_score);
static int count_free_straight(char grid[TRON_H][TRON_W + 1], int x, int y, dir_t d);
static dir_t bot_choose_dir(char grid[TRON_H][TRON_W + 1], int x, int y, dir_t cur_dir);
static void death_jingle_start(void);
static void death_jingle_stop(void);
static void death_jingle_update(void);
/*---------------------------*/

static void pause_ticks(int t){
    volatile int i, j;
    for (i = 0; i < t; ++i)
        for (j = 0; j < 1000; ++j)
            ;
}

static int death_jingle_is_active(void) {
    return death_jingle_state.active;
}

static void death_jingle_stop(void)
{
    if (!death_jingle_state.active)
        return;
    death_jingle_state.active = 0;
    death_jingle_state.index = 0;
    death_jingle_state.next_change_ns = 0;
    audioStopTone();
}

static void death_jingle_update(void)
{
    if (!death_jingle_state.active)
        return;

    const size_t count = sizeof(death_jingle_sequence) / sizeof(death_jingle_sequence[0]);
    if (count == 0) {
        death_jingle_stop();
        return;
    }

    long long now = actualTime();
    if (death_jingle_state.next_change_ns != 0 && now < death_jingle_state.next_change_ns)
        return;

    if (death_jingle_state.index >= count) {
        death_jingle_stop();
        return;
    }

    const tone_t tone = death_jingle_sequence[death_jingle_state.index];
    if (tone.freq == 0)
        audioStopTone();
    else
        audioPlayTone(tone.freq);

    long long duration = (long long)tone.duration_ms * 1000000LL;
    if (duration <= 0)
        duration = 50000000LL; // fallback 50ms
    death_jingle_state.next_change_ns = now + duration;
    death_jingle_state.index++;
}

static void death_jingle_start(void)
{
    death_jingle_state.active = 1;
    death_jingle_state.index = 0;
    death_jingle_state.next_change_ns = 0;
    death_jingle_update();
}


static void show_game_over(int result, player_score_t *p1_score, player_score_t *p2_score)
{
      // Cache de valores de pantalla para evitar múltiples llamadas
      static int cached_scr_w = 0;
      static int cached_scr_h = 0;
      static int cached_center_x = 0;
      static int cached_center_y = 0;
      
      int scr_w = get_screen_px_width();
      int scr_h = get_screen_px_height();
      if (scr_w <= 0) scr_w = 640;
      if (scr_h <= 0) scr_h = 480;
      
      // Only recalculate if resolution changed
      if (cached_scr_w != scr_w || cached_scr_h != scr_h) {
          cached_scr_w = scr_w;
          cached_scr_h = scr_h;
          cached_center_x = scr_w / 2;
          cached_center_y = scr_h / 2;
      }

    // Semi-transparent overlay
    fill_rect_blend(0, 0, scr_w, scr_h, 0x000000, 255);
    
    int center_x = scr_w / 2;
    int center_y = scr_h / 2;
    
     // Box dimensions - calculate once
     int is_multiplayer = (p1_score && p2_score);
     int box_w = is_multiplayer ? 400 : 300;
     int box_h = is_multiplayer ? 200 : 140; // Más alto para multiplayer
     int box_x = cached_center_x - box_w / 2;
     int box_y = cached_center_y - box_h / 2;


    static const struct {
        const char *msg;
        uint32_t border_color;
        uint32_t msg_color;
    } result_data[] = {
        {"GAME OVER", 0xFF4D00, 0xFFFF00},     // 0: Single player
        {"PLAYER 1 WINS!", 0xFF6600, 0xFF6600}, // 1: P1 wins
        {"PLAYER 2 WINS!", 0x00FF66, 0x00FF66}, // 2: P2 wins
        {"DRAW!", 0xFFFF00, 0xFFFF00},         // 3: Draw
        {"BOT WINS!", 0x00FF66, 0x00FF66},      // 4: Bot wins
        {"ERROR", 0xFF0000, 0xFF0000}          // default/error
    };

    int result_idx = (result >= 0 && result <= 4) ? result : 5;
    const char *main_msg = result_data[result_idx].msg;
    uint32_t border_color = result_data[result_idx].border_color;
    uint32_t msg_color = result_data[result_idx].msg_color;

    
    // Draw box with colored border
    fill_rect_blend(box_x - 3, box_y - 3, box_w + 6, box_h + 6, border_color, 255);
    fill_rect_blend(box_x, box_y, box_w, box_h, 0x000000, 255);
    
    // Main message centered
    int msg_w = text_measure_width(main_msg, 3);
    int msg_x = center_x - msg_w / 2;
    int msg_y = center_y - 35;
    
    text_draw_string(msg_x, msg_y, main_msg, 3, msg_color);



    if (p1_score && p2_score) {
        char score_text[64];
        int idx = 0;
        
        // "P1: XX  P2: XX"
        score_text[idx++] = 'P';
        score_text[idx++] = '1';
        score_text[idx++] = ':';
        score_text[idx++] = ' ';
        
        // P1 score
        if (p1_score->score >= 100) score_text[idx++] = '0' + (p1_score->score / 100);
        if (p1_score->score >= 10) score_text[idx++] = '0' + ((p1_score->score / 10) % 10);
        score_text[idx++] = '0' + (p1_score->score % 10);
        
        score_text[idx++] = ' ';
        score_text[idx++] = ' ';
        score_text[idx++] = 'P';
        score_text[idx++] = '2';
        score_text[idx++] = ':';
        score_text[idx++] = ' ';
        
        // P2 score
        if (p2_score->score >= 100) score_text[idx++] = '0' + (p2_score->score / 100);
        if (p2_score->score >= 10) score_text[idx++] = '0' + ((p2_score->score / 10) % 10);
        score_text[idx++] = '0' + (p2_score->score % 10);
        
        score_text[idx] = 0;
        
        int score_w = text_measure_width(score_text, 2);
        int score_x = center_x - score_w / 2;
        int score_y = center_y;
        text_draw_string(score_x, score_y, score_text, 2, 0xFFFFFF);
    }

    
    // Controls - precalculate positions
    int controls_y = cached_center_y + 30;
    
    // Control strings cache
    static const char *restart_text = "R = RESTART";
    static const char *menu_text = "ENTER = MENU";
    static int restart_w = 0;
    static int menu_w = 0;
    
    // Calculate widths once
    if (restart_w == 0) {
        restart_w = text_measure_width(restart_text, 2);
        menu_w = text_measure_width(menu_text, 2);
    }
    
    int restart_x = cached_center_x - restart_w / 2;
    int menu_x = cached_center_x - menu_w / 2;
    
    text_draw_string(restart_x, controls_y, restart_text, 2, 0x00FF00);
    text_draw_string(menu_x, controls_y + 25, menu_text, 2, 0x00AAFF);

}



static void draw_game_arena(int scr_w, int scr_h, int offset_x, int offset_y, int cell, int grid_w_px, int grid_h_px)
{
    /* Draw white background */
    fill_rect_blend(0, 0, scr_w, scr_h, 0xFFFFFF, 255);
    
    int border_th = cell / 4;
    if (border_th < 2) border_th = 2;
        
    /* Top border - EN la primera fila */
    fill_rect_blend(offset_x, offset_y, grid_w_px, border_th, 0x000000, 255);
    
    /* Bottom border - EN la última fila */
    fill_rect_blend(offset_x, offset_y + grid_h_px - border_th, grid_w_px, border_th, 0x000000, 255);
    
    /* Left border - EN la primera columna */
    fill_rect_blend(offset_x, offset_y, border_th, grid_h_px, 0x000000, 255);
    
    /* Right border - EN la última columna */
    fill_rect_blend(offset_x + grid_w_px - border_th, offset_y, border_th, grid_h_px, 0x000000, 255);
}



static void draw_trail_and_head(int offset_x, int offset_y, int cell, int old_x, int old_y, int new_x, int new_y, uint32_t color, int headFlag)
{
    static int cached_cell = 0;
    static int cached_half_cell = 0;
    static int cached_connector_pad = 0;
    static int cached_thickness = 0;
    static int cached_head_size = 0;
    
    // Only recalculate if cell size changed
    if (cached_cell != cell) {
        cached_cell = cell;
        cached_half_cell = cell >> 1;
        cached_connector_pad = cell / 6;
        if (cached_connector_pad < 1) cached_connector_pad = 1;
        cached_thickness = cell / 3;
        if (cached_thickness < 2) cached_thickness = 2;
        cached_head_size = cell / 3;
        if (cached_head_size < 1) cached_head_size = 1;
    }

    //Calculate positions of centers directly
    int prev_cx = offset_x + old_x * cell + cached_half_cell;
    int prev_cy = offset_y + old_y * cell + cached_half_cell;
    int new_cx = offset_x + new_x * cell + cached_half_cell;
    int new_cy = offset_y + new_y * cell + cached_half_cell;
    
    int is_horizontal = (prev_cy == new_cy);
    
    if (is_horizontal) {
        int left_x = (prev_cx < new_cx) ? prev_cx : new_cx;
        int right_x = (prev_cx < new_cx) ? new_cx : prev_cx;
        
        int x0 = left_x - cached_connector_pad;
        int width = right_x - left_x + (cached_connector_pad << 1);
        int y = prev_cy - (cached_thickness >> 1);
        
        // Boundary check 
        y = (y < 0) ? 0 : y;
        
        fill_rect_blend(x0, y, width, cached_thickness, color, 255);
    } else {
        int top_y = (prev_cy < new_cy) ? prev_cy : new_cy;
        int bottom_y = (prev_cy < new_cy) ? new_cy : prev_cy;
        
        int y0 = top_y - cached_connector_pad;
        int height = bottom_y - top_y + (cached_connector_pad << 1);
        int x = prev_cx - (cached_thickness >> 1);
        
        x = (x < 0) ? 0 : x;
        
        fill_rect_blend(x, y0, cached_thickness, height, color, 255);
    }

    /* Head drawing */
    if (headFlag) {
        int hx = offset_x + new_x * cell + ((cell - cached_head_size) >> 1);
        int hy = offset_y + new_y * cell + ((cell - cached_head_size) >> 1);
        
        fill_rect_blend(hx, hy, cached_head_size, cached_head_size, color, 255);
    }
}



static void format_fps_label(unsigned fps_value, char *out, int out_cap)
{
    // Fast boundary check
    if (out_cap < 6) { 
        if (out_cap > 0) out[0] = 0;
        return;
    }
    
    // Constant prefix - faster than loops
    out[0] = 'F';
    out[1] = 'P';
    out[2] = 'S';
    out[3] = ':';
    out[4] = ' ';
    
    int idx = 5;
    
    if (fps_value == 0) {
        if (idx + 2 < out_cap) {
            out[idx++] = '-';
            out[idx++] = '-';
            out[idx] = 0;
        }
        return;
    }
    
    if (fps_value > 999) fps_value = 999;
    
    // Optimized conversion using lookup and division
    int remaining_space = out_cap - idx - 1; 
    
    if (fps_value >= 100 && remaining_space >= 3) {
        // 3 digits
        out[idx++] = '0' + (fps_value / 100);
        out[idx++] = '0' + ((fps_value / 10) % 10);
        out[idx++] = '0' + (fps_value % 10);
    } else if (fps_value >= 10 && remaining_space >= 2) {
        // 2 digits
        out[idx++] = '0' + (fps_value / 10);
        out[idx++] = '0' + (fps_value % 10);
    } else if (remaining_space >= 1) {
        // 1 digit
        out[idx++] = '0' + (fps_value % 10);
    }
    
    out[idx] = 0;
}


static void draw_fps_overlay(int scr_w, int top_margin, unsigned fps_value)
{
    // Cache simple - solo variables esenciales
    static char cached_label[12];        // "FPS: XXX" máximo
    static unsigned last_fps = 9999;     // Forzar primera actualización
    static int cached_x = 0;
    static int cached_y = 0;
    static int cached_w = 0;
    static int cached_h = 0;
    static int last_scr_w = 0;
    
    // Constantes fijas
    const int scale = 3;
    const int padding = 4;
    
    // Solo actualizar si FPS o ancho de pantalla cambió
    int fps_changed = (fps_value != last_fps);
    int screen_changed = (scr_w != last_scr_w);
    
    if (fps_changed) {
        // Generar label simple
        format_fps_label(fps_value, cached_label, sizeof(cached_label));
        last_fps = fps_value;
        screen_changed = 1; // Forzar recálculo de posición
    }
    
    if (screen_changed) {
        // Recalcular posición y dimensiones
        int text_w = text_measure_width(cached_label, scale);
        int text_h = text_measure_height(scale);
        
        cached_w = text_w + (padding << 1);  // padding * 2
        cached_h = text_h + (padding << 1);
        
        cached_x = scr_w - cached_w - 8;
        if (cached_x < 0) cached_x = 0;
        
        cached_y = (top_margin > cached_h) ? (top_margin - cached_h) >> 1 : 4; // /2
        
        last_scr_w = scr_w;
    }
    
    // Dibujar siempre (simple y directo)
    fill_rect_blend(cached_x, cached_y, cached_w, cached_h, 0x000000, 180);
    text_draw_string(cached_x + padding, cached_y + padding, cached_label, scale, 0xFFFFFF);
}



static void show_controls_and_init_fps(fps_counter_t *fps_counter, int is_multiplayer) {

    // Cache de geometría de pantalla
    static int cached_scr_w = 0;
    static int cached_scr_h = 0;

    int scr_w = get_screen_px_width();
    int scr_h = get_screen_px_height();
    if (scr_w <= 0) scr_w = 640;
    if (scr_h <= 0) scr_h = 480;
       // Solo recalcular posiciones si cambió la resolución
    int geometry_changed = (cached_scr_w != scr_w || cached_scr_h != scr_h);
    
    // Variables de layout - precalculadas
    static int cached_x = 80;
    static int cached_y = 80;
    static const int spacing = 40;
    
    if (geometry_changed) {
        cached_x = 80;
        cached_y = 80;
        cached_scr_w = scr_w;
        cached_scr_h = scr_h;
    }
    
    // Fondo negro - una sola llamada
    fill_rect_blend(0, 0, scr_w, scr_h, 0x000000, 255);

    
    int x = cached_x;
    int y = cached_y;
    
    if (is_multiplayer) {
        // Título simple
        text_draw_string(x, y, "MULTIPLAYER", 4, 0xFFFFFF);
        y += 70;
        
        // Controles más simples
        text_draw_string(x, y, "P1: WASD", 3, 0xFF6600);
        y += spacing;
        
        text_draw_string(x, y, "P2: IJKL", 3, 0x00FF66);
        y += spacing;
        
        text_draw_string(x, y, "QUIT: Q", 2, 0xFFFFFF);
        
    } else {
        // Single player - súper simple
        text_draw_string(x, y, "SINGLE PLAYER", 4, 0xFFFFFF);
        y += 70;
        
        text_draw_string(x, y, "MOVE: WASD", 3, 0xFF6600);
        y += spacing;
        
        text_draw_string(x, y, "QUIT: Q", 2, 0xFFFFFF);
    }
    
    // Loading simple
    text_draw_string(x, scr_h - 100, "LOADING...", 2, 0x888888);
    
    // Calibrar FPS
    fps_counter_init(fps_counter);
    

    
        // Blinking ready message - OPTIMIZADO
        int blink_counter = 0;
        const int blink_interval = 30; // Constante para mejor performance
        const int clear_area_x = x - 10;
        const int clear_area_y = scr_h - 130;
        const int clear_area_w = 400;
        const int clear_area_h = 40;
        const int ready_text_x = x;
        const int ready_text_y = scr_h - 120;
    
        

        while (1) {
            // Clear the loading area and show ready message
            fill_rect_blend(clear_area_x, clear_area_y, clear_area_w, clear_area_h, 0x000000, 255);
            
            // Blinking effect optimizado - usar bit operations
            if ((blink_counter / blink_interval) & 1) { // & 1 es más rápido que % 2
                text_draw_string(ready_text_x, ready_text_y, "PRESS ENTER TO START", 2, 0x00FF00);
            }
            
            blink_counter++;
            
            // Check for ENTER key
            if (kbd_available()) {
                char c = 0;
                if (read(0, &c, 1) > 0) {
                    if (c == '\n' || c == '\r') {
                        return; // Start the game!
                    }
                    // Allow quitting before game starts
                    if (c == 'q' || c == 'Q') {
                        // Exit to menu - you might want to handle this differently
                        return;
                    }
                }
            }
            
            // Small delay for blinking effect
            pause_ticks(3);
        }
}


// Non-blocking input processing for single player
static inline void tron_read_input_single(dir_t *dir, int *alive_flag) {
    while (kbd_available()) {
        char c;
        if (read(0, &c, 1) <= 0) break;
        if (c == 'q' || c == 'Q') { *alive_flag = 0; return; }
        switch (c) {
            case 'w': if (*dir != DIR_DOWN)  *dir = DIR_UP;    break;
            case 's': if (*dir != DIR_UP)    *dir = DIR_DOWN;  break;
            case 'a': if (*dir != DIR_RIGHT) *dir = DIR_LEFT;  break;
            case 'd': if (*dir != DIR_LEFT)  *dir = DIR_RIGHT; break;
            default: break;
        }
    }
}

// Non-blocking input processing for multiplayer
static inline void tron_read_input_multi(dir_t *p1_dir, int *p1_alive, dir_t *p2_dir, int *p2_alive) {
    while (kbd_available()) {
        char c;
        if (read(0, &c, 1) <= 0) break;
        if (c == 'q' || c == 'Q') { *p1_alive = 0; *p2_alive = 0; return; }
        switch (c) {
            // P1 WASD
            case 'w': if (*p1_dir != DIR_DOWN)  *p1_dir = DIR_UP;    break;
            case 's': if (*p1_dir != DIR_UP)    *p1_dir = DIR_DOWN;  break;
            case 'a': if (*p1_dir != DIR_RIGHT) *p1_dir = DIR_LEFT;  break;
            case 'd': if (*p1_dir != DIR_LEFT)  *p1_dir = DIR_RIGHT; break;
            // P2 IJKL
            case 'i': if (*p2_dir != DIR_DOWN)  *p2_dir = DIR_UP;    break;
            case 'k': if (*p2_dir != DIR_UP)    *p2_dir = DIR_DOWN;  break;
            case 'j': if (*p2_dir != DIR_RIGHT) *p2_dir = DIR_LEFT;  break;
            case 'l': if (*p2_dir != DIR_LEFT)  *p2_dir = DIR_RIGHT; break;
            default: break;
        }
    }
}

static int count_free_straight(char grid[TRON_H][TRON_W + 1], int x, int y, dir_t d) {
    static const int dx[4] = {0, 1, 0, -1};
    static const int dy[4] = {-1, 0, 1, 0};
    int cx = x, cy = y;
    int cnt = 0;
    while (1) {
        cx += dx[d];
        cy += dy[d];
        if (cx < 0 || cx >= TRON_W || cy < 0 || cy >= TRON_H)
            break;
        if (grid[cy][cx] != ' ')
            break;
        cnt++;
    }
    return cnt;
}

static dir_t bot_choose_dir(char grid[TRON_H][TRON_W + 1], int x, int y, dir_t cur_dir) {
    dir_t forward = cur_dir;
    dir_t left    = left_of(cur_dir);
    dir_t right   = right_of(cur_dir);
    dir_t back    = (dir_t)((cur_dir + 2) & 3);

    int f_free = count_free_straight(grid, x, y, forward);
    int l_free = count_free_straight(grid, x, y, left);
    int r_free = count_free_straight(grid, x, y, right);

    if (f_free > 0) return forward;

    if (l_free > r_free && l_free > 0) return left;
    if (r_free > l_free && r_free > 0) return right;

    if (l_free > 0) return left;
    if (r_free > 0) return right;

    int b_free = count_free_straight(grid, x, y, back);
    if (b_free > 0) return back;

    return cur_dir;
}

// ========== TRON-SINGLE PLAYER ==========
static int tron_single_impl(void)
{
    // FPS + sound
    fps_counter_t fps_counter;
    show_controls_and_init_fps(&fps_counter, 0);
    fps_counter_init(&fps_counter);

    static char grid[TRON_H][TRON_W + 1];

    // Init grid
    for (int y = 0; y < TRON_H; ++y) {
        for (int x = 0; x < TRON_W; ++x) 
            grid[y][x] = ' ';
        grid[y][TRON_W] = '\n';
    }

    // Init player
    int px = TRON_W >> 2;
    int py = TRON_H >> 1;
    dir_t pdir = DIR_RIGHT;
    int palive = 1;
    const uint32_t pcolor = 0xFF4D00; // Naranja
    
    //Init bot
    int bx = (TRON_W * 3) >> 2;
    int by = TRON_H >> 1;
    dir_t bdir = DIR_LEFT;
    int balive = 1;
    const uint32_t bcolor = 0x00FF4D; // Verde
    
    grid[py][px] = '1';
    grid[by][bx] = '2';
    
    int scr_w = get_screen_px_width();
    int scr_h = get_screen_px_height();
    if (scr_w <= 0) 
        scr_w = 640;
    if (scr_h <= 0) 
        scr_h = 480;

    int cell_w = scr_w / TRON_W;
    int cell_h = scr_h / TRON_H;
    int est_cell = (cell_w < cell_h) ? cell_w : cell_h;
    if (est_cell < 1) 
        est_cell = 1;

    int top_margin = est_cell << 1;
    int bottom_margin = est_cell;
    int available_h = scr_h - top_margin - bottom_margin;
    if (available_h < TRON_H) 
        available_h = TRON_H;

    int cell = scr_w / TRON_W;
    int cell_from_h = available_h / TRON_H;
    if (cell_from_h < cell) 
        cell = cell_from_h;
    if (cell < 1) 
        cell = 1;
    
    // Timestep derivado de velocidad en píxeles/seg
    long long step_ns = (1000000000LL * (long long)cell) / (long long)TARGET_PX_PER_SEC;

    int offset_x = (scr_w - (cell * TRON_W)) >> 1;
    int offset_y = top_margin + ((available_h - cell * TRON_H) >> 1);
    if (offset_y < top_margin) 
        offset_y = top_margin;

    // Fondo y bordes
    draw_game_arena(scr_w, scr_h, offset_x, offset_y, cell, cell * TRON_W, cell * TRON_H);

    // Cabeza inicial
    int pad = cell >> 3; 
    if (pad < 1) 
        pad = 1;
    int hs = cell - (pad << 1);
    fill_rect_blend(offset_x + px * cell + pad, offset_y + py * cell + pad, hs, hs, pcolor, 255);
    fill_rect_blend(offset_x + bx * cell + pad, offset_y + by * cell + pad, hs, hs, bcolor, 255);

    draw_fps_overlay(scr_w, top_margin, 0);
    death_jingle_stop();

    // Deadline init
    long long next_step_ns = actualTime();
    unsigned fps_value = 0;
    int frame_count = 0;

    static const int dx[4] = {0, 1, 0, -1}; // UP, RIGHT, DOWN, LEFT
    static const int dy[4] = {-1, 0, 1, 0};

    while (palive && balive) {
            death_jingle_update();
    
            // Idle until deadline: input player
            long long now = actualTime();
            while (now < next_step_ns) {
                while (kbd_available()) {
                    char c;
                    if (read(0, &c, 1) <= 0) break;
                    if (c == 'q' || c == 'Q') { palive = 0; balive = 0; break; }
                    switch (c) {
                        case 'w': if (pdir != DIR_DOWN)  pdir = DIR_UP;    break;
                        case 's': if (pdir != DIR_UP)    pdir = DIR_DOWN;  break;
                        case 'a': if (pdir != DIR_RIGHT) pdir = DIR_LEFT;  break;
                        case 'd': if (pdir != DIR_LEFT)  pdir = DIR_RIGHT; break;
                        default: break;
                    }
                }
                if (!palive || !balive) break;
                death_jingle_update();
                now = actualTime();
            }
            if (!palive || !balive) break;
    
            int catchup = 0;
            do {
                // FPS
                fps_value = fps_counter_tick(&fps_counter);
                if ((frame_count & (FPS_UPDATE_INTERVAL - 1)) == 0) {
                    draw_fps_overlay(scr_w, top_margin, fps_value);
                }
                frame_count++;
    
                // 1) Player
                if (palive) {
                    int npx = px + dx[pdir];
                    int npy = py + dy[pdir];
    
                    if (npx < 0 || npx >= TRON_W || npy < 0 || npy >= TRON_H || grid[npy][npx] != ' ') {
                        if (npx >= 0 && npx < TRON_W && npy >= 0 && npy < TRON_H)
                            draw_trail_and_head(offset_x, offset_y, cell, px, py, npx, npy, pcolor, 0);
                        palive = 0;
                        if (!death_jingle_is_active()) death_jingle_start();
                    } else {
                        draw_trail_and_head(offset_x, offset_y, cell, px, py, npx, npy, pcolor, 1);
                        if (py == npy) {
                            int sx = (px < npx) ? px : npx;
                            int ex = (px < npx) ? npx : px;
                            char *row = grid[py];
                            for (int mx = sx; mx <= ex; ++mx) row[mx] = '1';
                        } else {
                            int sy = (py < npy) ? py : npy;
                            int ey = (py < npy) ? npy : py;
                            for (int my = sy; my <= ey; ++my) grid[my][px] = '1';
                        }
                        px = npx; py = npy;
                    }
                }
    
                // 2) Bot
                if (balive) {
                    bdir = bot_choose_dir(grid, bx, by, bdir);
    
                    int nbx = bx + dx[bdir];
                    int nby = by + dy[bdir];
    
                    if (nbx < 0 || nbx >= TRON_W || nby < 0 || nby >= TRON_H || grid[nby][nbx] != ' ') {
                        if (nbx >= 0 && nbx < TRON_W && nby >= 0 && nby < TRON_H)
                            draw_trail_and_head(offset_x, offset_y, cell, bx, by, nbx, nby, bcolor, 0);
                        balive = 0;
                        if (!death_jingle_is_active()) death_jingle_start();
                    } else {
                        draw_trail_and_head(offset_x, offset_y, cell, bx, by, nbx, nby, bcolor, 1);
                        if (by == nby) {
                            int sx = (bx < nbx) ? bx : nbx;
                            int ex = (bx < nbx) ? nbx : bx;
                            char *row = grid[by];
                            for (int mx = sx; mx <= ex; ++mx) row[mx] = '2';
                        } else {
                            int sy = (by < nby) ? by : nby;
                            int ey = (by < nby) ? nby : by;
                            for (int my = sy; my <= ey; ++my) grid[my][bx] = '2';
                        }
                        bx = nbx; by = nby;
                    }
                }
    
                next_step_ns += step_ns;
                now = actualTime();
                
                } while (palive && balive && now >= next_step_ns && ++catchup < MAX_CATCHUP_STEPS);
    }

    death_jingle_update();
    if (palive && !balive){
        show_game_over(1, NULL, NULL); // Player
    } else if (!palive && balive) {
        show_game_over(4, NULL, NULL); // Bot
    } else {
        show_game_over(3, NULL, NULL); // Draw
    }

    // Wait for restart or return to menu
    while (1) {
        death_jingle_update();
        if (kbd_available()) {
            char c = 0;
            if (read(0, &c, 1) > 0) {
                if (c == '\n' || c == '\r') {
                    death_jingle_stop();
                    return 0; // exit to menu
                } else if (c == 'r' || c == 'R') {
                    death_jingle_stop();
                    return 1; // restart
                }
            }
        }
        pause_ticks(2);
    }
}


// ========== TRON-MULTIPLAYER ==========
static int tron_multi_impl(void)
{
    // Points accumulated between games
    static player_score_t p1_score = {0};
    static player_score_t p2_score = {0};

    fps_counter_t fps_counter;
    show_controls_and_init_fps(&fps_counter, 1);
    fps_counter_init(&fps_counter);

    static char grid[TRON_H][TRON_W + 1];
    for (int y = 0; y < TRON_H; ++y) {
        char *row = grid[y];
        for (int x = 0; x < TRON_W; ++x) row[x] = ' ';
        row[TRON_W] = '\n';
    }

    // Initial positions
    int p1x = TRON_W >> 2;
    int p1y = TRON_H >> 1;
    dir_t p1_dir = DIR_RIGHT;
    int p1_alive = 1;
    static const uint32_t p1_color = 0xFF4D00;

    int p2x = (TRON_W * 3) >> 2;
    int p2y = TRON_H >> 1;
    dir_t p2_dir = DIR_LEFT;
    int p2_alive = 1;
    static const uint32_t p2_color = 0x00FF4D;

    grid[p1y][p1x] = '1';
    grid[p2y][p2x] = '2';

    // Geometry
    int scr_w = get_screen_px_width();
    int scr_h = get_screen_px_height();
    if (scr_w <= 0) scr_w = 640;
    if (scr_h <= 0) scr_h = 480;

    int cell_w = scr_w / TRON_W;
    int cell_h = scr_h / TRON_H;
    int est_cell = (cell_w < cell_h) ? cell_w : cell_h;
    if (est_cell < 1) est_cell = 1;

    int top_margin = est_cell << 1;
    int bottom_margin = est_cell;
    int available_h = scr_h - top_margin - bottom_margin;
    if (available_h < TRON_H) available_h = TRON_H;

    int cell = scr_w / TRON_W;
    int cell_from_h = available_h / TRON_H;
    if (cell_from_h < cell) cell = cell_from_h;
    if (cell < 1) cell = 1;
    long long step_ns = (1000000000LL * (long long)cell) / (long long)TARGET_PX_PER_SEC;


    int offset_x = (scr_w - (cell * TRON_W)) >> 1;
    int offset_y = top_margin + ((available_h - cell * TRON_H) >> 1);
    if (offset_y < top_margin) offset_y = top_margin;

    // Background and initial heads
    draw_game_arena(scr_w, scr_h, offset_x, offset_y, cell, cell * TRON_W, cell * TRON_H);
    int pad = cell >> 3; if (pad < 1) pad = 1;
    int hs = cell - (pad << 1);
    fill_rect_blend(offset_x + p1x * cell + pad, offset_y + p1y * cell + pad, hs, hs, p1_color, 255);
    fill_rect_blend(offset_x + p2x * cell + pad, offset_y + p2y * cell + pad, hs, hs, p2_color, 255);

    draw_fps_overlay(scr_w, top_margin, 0);
    death_jingle_stop();

    // Fixed timestep
    long long next_step_ns = actualTime();
    unsigned fps_value = 0;
    int frame_count = 0;

    static const int dx[4] = {0, 1, 0, -1};
    static const int dy[4] = {-1, 0, 1, 0};

    while (p1_alive && p2_alive) {
        death_jingle_update();

        // Idle until deadline: drain input from both players
        long long now = actualTime();
        while (now < next_step_ns) {
            while (kbd_available()) {
                char c;
                if (read(0, &c, 1) <= 0) break;
                if (c == 'q' || c == 'Q') { p1_alive = 0; p2_alive = 0; break; }
                // P1 WASD
                switch (c) {
                    case 'w': if (p1_dir != DIR_DOWN)  p1_dir = DIR_UP;    break;
                    case 's': if (p1_dir != DIR_UP)    p1_dir = DIR_DOWN;  break;
                    case 'a': if (p1_dir != DIR_RIGHT) p1_dir = DIR_LEFT;  break;
                    case 'd': if (p1_dir != DIR_LEFT)  p1_dir = DIR_RIGHT; break;
                    default: break;
                }
                // P2 IJKL
                switch (c) {
                    case 'i': if (p2_dir != DIR_DOWN)  p2_dir = DIR_UP;    break;
                    case 'k': if (p2_dir != DIR_UP)    p2_dir = DIR_DOWN;  break;
                    case 'j': if (p2_dir != DIR_RIGHT) p2_dir = DIR_LEFT;  break;
                    case 'l': if (p2_dir != DIR_LEFT)  p2_dir = DIR_RIGHT; break;
                    default: break;
                }
            }
            if (!p1_alive || !p2_alive) break;
            death_jingle_update();
            now = actualTime();
        }
        if (!p1_alive || !p2_alive) break;

        int catchup = 0;
        do {
            // FPS
            fps_value = fps_counter_tick(&fps_counter);
            if ((frame_count & (FPS_UPDATE_INTERVAL - 1)) == 0) {
                draw_fps_overlay(scr_w, top_margin, fps_value);
            }
            frame_count++;

            // Move P1
            if (p1_alive) {
                int np1x = p1x + dx[p1_dir];
                int np1y = p1y + dy[p1_dir];
                if (np1x < 0 || np1x >= TRON_W || np1y < 0 || np1y >= TRON_H || grid[np1y][np1x] != ' ') {
                    if (np1x >= 0 && np1x < TRON_W && np1y >= 0 && np1y < TRON_H)
                        draw_trail_and_head(offset_x, offset_y, cell, p1x, p1y, np1x, np1y, p1_color, 0);
                    p1_alive = 0;
                    p2_score.score += 10;
                    if (!death_jingle_is_active()) death_jingle_start();
                } else {
                    draw_trail_and_head(offset_x, offset_y, cell, p1x, p1y, np1x, np1y, p1_color, 1);
                    if (p1y == np1y) {
                        int sx = (p1x < np1x) ? p1x : np1x;
                        int ex = (p1x < np1x) ? np1x : p1x;
                        char *row = grid[p1y];
                        for (int mx = sx; mx <= ex; ++mx) row[mx] = '1';
                    } else {
                        int sy = (p1y < np1y) ? p1y : np1y;
                        int ey = (p1y < np1y) ? np1y : p1y;
                        for (int my = sy; my <= ey; ++my) grid[my][p1x] = '1';
                    }
                    p1x = np1x; p1y = np1y;
                }
            }

            // Move P2
            if (p2_alive) {
                int np2x = p2x + dx[p2_dir];
                int np2y = p2y + dy[p2_dir];
                if (np2x < 0 || np2x >= TRON_W || np2y < 0 || np2y >= TRON_H || grid[np2y][np2x] != ' ') {
                    if (np2x >= 0 && np2x < TRON_W && np2y >= 0 && np2y < TRON_H)
                        draw_trail_and_head(offset_x, offset_y, cell, p2x, p2y, np2x, np2y, p2_color, 0);
                    p2_alive = 0;
                    p1_score.score += 10;
                    if (!death_jingle_is_active()) death_jingle_start();
                } else {
                    draw_trail_and_head(offset_x, offset_y, cell, p2x, p2y, np2x, np2y, p2_color, 1);
                    if (p2y == np2y) {
                        int sx = (p2x < np2x) ? p2x : np2x;
                        int ex = (p2x < np2x) ? np2x : p2x;
                        char *row = grid[p2y];
                        for (int mx = sx; mx <= ex; ++mx) row[mx] = '2';
                    } else {
                        int sy = (p2y < np2y) ? p2y : np2y;
                        int ey = (p2y < np2y) ? np2y : p2y;
                        for (int my = sy; my <= ey; ++my) grid[my][p2x] = '2';
                    }
                    p2x = np2x; p2y = np2y;
                }
            }

            next_step_ns += step_ns;
            now = actualTime();

        } while (p1_alive && p2_alive && now >= next_step_ns && ++catchup < MAX_CATCHUP_STEPS);
    }

    // Show result
    death_jingle_update();
    if (p1_alive && !p2_alive)      show_game_over(1, &p1_score, &p2_score);
    else if (!p1_alive && p2_alive) show_game_over(2, &p1_score, &p2_score);
    else                            show_game_over(3, &p1_score, &p2_score);

    // Wait for restart or exit
    while (1) {
        death_jingle_update();
        if (kbd_available()) {
            char c = 0;
            if (read(0, &c, 1) > 0) {
                if (c == '\n' || c == '\r') {
                    death_jingle_stop();
                    return 0; // exit to menu
                } else if (c == 'r' || c == 'R') {
                    death_jingle_stop();
                    return 1; // restart
                }
            }
        }
        pause_ticks(2);
    }
}


/*-----------------TRON-MENU-----------------------*/
void tron_menu(void)
{
    while (kbd_available())
    {
        char tmp;
        if (read(0, &tmp, 1) <= 0)
            break;
    }
       
    int cols = get_shell_cols();
    int rows = get_shell_rows();

    const char *lines[] = {"--- TRON ---", "", "1) Single player", "2) Multiplayer", "e) Back to shell"};
    int nlines = (int)(sizeof(lines) / sizeof(lines[0]));

    set_colors(0x000000, 0xFFFFFF); // black text on white background

    clear_screen();

    /* layout */
    const int border = 1;
    const int inner_pad_h = 1;
    const int inner_pad_w = 2;

    int maxlen = 0;
    for (int i = 0; i < nlines; ++i)
    {
        int l = 0;
        while (lines[i][l])
            l++;
        if (l > maxlen)
            maxlen = l;
    }

    int box_w = maxlen + 2 * inner_pad_w + 2 * border;
    int box_h = nlines + 2 * inner_pad_h + 2 * border;
    if (box_w > cols)
        box_w = cols;
    if (box_h > rows)
        box_h = rows;

    int x0 = (cols - box_w) / 2;
    int y0 = (rows - box_h) / 2;

    enum
    {
        LINEBUF_MAX = 1024
    };
    static char spaces[LINEBUF_MAX];
    int cap = cols;
    if (cap > LINEBUF_MAX - 2)
        cap = LINEBUF_MAX - 2;
    for (int i = 0; i < cap; ++i)
        spaces[i] = ' ';

    // Simple rendering: clear screen, scroll start_y lines, then print only box_h rows.
    int start_y = y0;
    for (int i = 0; i < start_y; ++i)
        write(1, "\n", 1);

    for (int r = 0; r < box_h; ++r)
    {
        /* left padding */
        int lw = x0;
        while (lw > 0)
        {
            int w = lw > cap ? cap : lw;
            write(1, spaces, w);
            lw -= w;
        }

        // Print centered text line
        int text_row = r - border - inner_pad_h;
        if (text_row >= 0 && text_row < nlines)
        {
            const char *txt = lines[text_row];
            int tlen = 0;
            while (txt[tlen])
                tlen++;
            int pad_left = (box_w - tlen) / 2;
            int bw = pad_left;
            while (bw > 0)
            {
                int w = bw > cap ? cap : bw;
                write(1, spaces, w);
                bw -= w;
            }
            /* ensure black text on white bg */
            set_colors(0x000000, 0xFFFFFF);
            write(1, txt, tlen);

            set_colors(0x000000, 0xFFFFFF);
            int aw = box_w - pad_left - tlen;
            while (aw > 0)
            {
                int w = aw > cap ? cap : aw;
                write(1, spaces, w);
                aw -= w;
            }
        }
        else
        {
            int tw = box_w;
            while (tw > 0)
            {
                int w = tw > cap ? cap : tw;
                write(1, spaces, w);
                tw -= w;
            }
        }

        write(1, "\n", 1);
    }

    int restartFlag = 1; //medio mal estilo
    /* wait for selection */
    while (1)
    {
        if (kbd_available())
        {
            char c = 0;
            if (read(0, &c, 1) > 0)
            {
                if (c == '1')
                {
                    /* start game: use game colors (white on black) */
                    set_colors(0xFFFFFF, 0x000000);

                    while(restartFlag) {
                        restartFlag = tron_single_impl();  // tron_single_impl() retorna 1 si reinicia, 0 si sale
                    }

                    /* after game, restore shell bg to project color 0x272827 */
                    set_colors(0xFFFFFF, 0x272827);
                    clear_screen();
                    return;
                }
                else if (c == '2')
                {
                   /* start multiplayer game */
                   set_colors(0xFFFFFF, 0x000000);
                    
                   restartFlag = 1;
                   while(restartFlag) {
                       restartFlag = tron_multi_impl();
                   }
                   
                   set_colors(0xFFFFFF, 0x272827);
                   clear_screen();
                   return;
                }
                else if (c == 'e' || c == 'q')
                {
                    /* clear the screen to shell background and return */
                    set_colors(0xFFFFFF, 0x272827);
                    clear_screen();
                    return;
                }
            }
        }
        pause_ticks(1);
    }
}
