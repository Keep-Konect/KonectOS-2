#include <global/console.h>

#include "ansi.h"

#include <kernel.h>
#include <lib/log.h>
#include <lib/lock.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <lib/bitmap.h>
#include <lib/memory.h>
#include <lib/assert.h>
#include <impl/serial.h>
#include <impl/initrd.h>

#define CONSOLE_VERSION "1.0"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

static bool use_boot_fb = false;
static spinlock_t boot_fb_lock = SPINLOCK_INIT;

static uint32_t bg_color;
static uint32_t fg_color;

static uint32_t* fb_base;
static uint16_t fb_width, fb_height;
static uint16_t fb_pitch;
static uint8_t fb_bpp;
static uint8_t fb_btpp;
static size_t fb_size;

static uint16_t line_count = 0;

// char position
static uint16_t cx_index;
static uint16_t cy_index;

static uint16_t cx_max_index;
static uint16_t cy_max_index;

static uint8_t* font_buffer;
static size_t font_size;

void console_set_bg_color(uint32_t bg) {
    bg_color = bg;
}
void console_set_fg_color(uint32_t fg) {
    fg_color = fg;
}

static void console_putpixel(uint16_t x, uint16_t y, uint32_t color) {
    fb_base[y*fb_width+x] = color;
}

static void console_setchar(uint16_t cx_ppos, uint16_t cy_ppos, char c){
    uint8_t* glyph = &font_buffer[c*((FONT_WIDTH*FONT_HEIGHT)/8)];
    
    for(uint16_t y = 0; y < FONT_HEIGHT; y++) {
        for(uint16_t x = 0; x < FONT_WIDTH; x++) {
            
            if(BIT_GET(glyph[y], FONT_WIDTH-x) == BITSET) {
                console_putpixel(cx_ppos+x, cy_ppos, fg_color); 
            }
        }
        cy_ppos++;
    }
}

static void console_clearchar(uint16_t cx_ppos, uint16_t cy_ppos){    
    for(uint16_t y = 0; y < FONT_HEIGHT; y++) {
        for(uint16_t x = 0; x < FONT_WIDTH; x++) {
            console_putpixel(cx_ppos+x, cy_ppos, bg_color); 
        }
        cy_ppos++;
    }
}

static void console_printline(uint16_t x, uint16_t line, char* str){
    for(size_t i = 0; i < strlen(str); i++) {
        console_setchar((x + i) * FONT_WIDTH, line * FONT_HEIGHT, str[i]);
    }
}

static void console_new_line(void){
    cx_index = 0;
    cy_index = ((cy_index + 1) % cy_max_index);
    uint16_t next_cy_index = ((cy_index + 1) % cy_max_index);

    line_count++;

    size_t line_size = (size_t)fb_pitch * (size_t)FONT_HEIGHT;
    size_t line_pixel_count = (size_t)fb_width * (size_t)FONT_HEIGHT; 
    void* fb_base_to_clear = (void*)((uintptr_t)fb_base + (uintptr_t)line_size * cy_index);
    memset32(fb_base_to_clear, bg_color, line_pixel_count);
    void* fb_base_to_cut = (void*)((uintptr_t)fb_base + (uintptr_t)line_size * next_cy_index);
    memset32(fb_base_to_cut, DEFAULT_CUT_COLOR, line_pixel_count);

    char cut_buffer[cx_max_index];
    snprintf_((char*)&cut_buffer, cx_max_index, "Kot version : %s | Kernel version : %s | Console version : %s | Number of lines written : %d", GIT_HASH, KERNEL_VERSION, CONSOLE_VERSION, line_count);
    console_printline(0, next_cy_index, cut_buffer);
}

static int boot_fb_callback(void){
    log_warning("console will no more use the framebuffer for this session\n");
    assert(!spinlock_acquire(&boot_fb_lock));
    use_boot_fb = false;
    spinlock_release(&boot_fb_lock);
    return 0;
}

void console_init(void) {
    void* font_file = initrd_get_file("/system/console/fonts/vga.bin");
    font_size = initrd_get_file_size(font_file);
    font_buffer = initrd_get_file_base(font_file);

    console_set_bg_color(DEFAULT_BG_COLOR);
    console_set_fg_color(DEFAULT_FG_COLOR);

    graphics_boot_fb_t* boot_fb = graphics_get_boot_fb(&boot_fb_callback);

    if(boot_fb == NULL){
        use_boot_fb = false;
    }else{
        if(boot_fb->bpp != 32){
            use_boot_fb = false;
        }else{
            use_boot_fb = true;

            fb_base = boot_fb->base;
            fb_width = boot_fb->width;
            fb_height = boot_fb->height;
            fb_pitch = boot_fb->pitch;
            fb_bpp = boot_fb->bpp;
            fb_btpp = boot_fb->btpp;
            fb_size = boot_fb->size;
        }
        cx_index = 0;
        cy_index = 0;

        cx_max_index = fb_width / FONT_WIDTH;
        cy_max_index = fb_height / FONT_HEIGHT;

        memset32(fb_base, bg_color, fb_size / sizeof(uint32_t));
    }
}

void console_putchar(char c) {
    assert(!spinlock_acquire(&boot_fb_lock));

    if(!use_boot_fb){
        spinlock_release(&boot_fb_lock);
        return;
    }

    // char pixel-position
    uint16_t cx_ppos = cx_index * FONT_WIDTH;
    if((cx_ppos + FONT_WIDTH) > fb_width) {
        console_new_line();
    }

    uint16_t cy_ppos = cy_index * FONT_HEIGHT;

    if(c == '\n') {
        console_new_line();
        spinlock_release(&boot_fb_lock);
        return;
    }
    if(c == '\r') {
        cx_index = 0;
        spinlock_release(&boot_fb_lock);
        return;
    }

    console_setchar(cx_ppos, cy_ppos, c);

    cx_index++;

    spinlock_release(&boot_fb_lock);
}

void console_delchar(void){
    assert(!spinlock_acquire(&boot_fb_lock));
    if(cx_index != 0){
        cx_index--;
    }else{
        cx_index = cx_max_index - 1;
        if(cy_index == 0){
            cy_index = cy_max_index - 1;
        }else{
            cy_index = (cy_index - 1) % cy_max_index;
        }
    }
    uint16_t cx_ppos = cx_index * FONT_WIDTH;
    uint16_t cy_ppos = cy_index * FONT_HEIGHT;
    console_clearchar(cx_ppos, cy_ppos);
    spinlock_release(&boot_fb_lock);
}

void console_print(const char* str) {
    for(size_t i = 0; i < strlen(str); i++) {
        if(str[i] == '\n'){
            console_putchar('\r');
            serial_write('\r');   
        }

        if(str[i] == '\e' || str[i] == '\033') {
            i = (size_t)ansi_read(str+i) + i;
        }

        console_putchar(str[i]);
        serial_write(str[i]);
    }
}