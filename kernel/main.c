#include "kernel.h"
#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num; // syms info
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} __attribute__((packed)) multiboot_info_t;

static volatile uint8_t* framebuffer = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_bpp = 0;
static int gui_mode = 0;

static volatile uint16_t* const VGA_BUFFER = (uint16_t*) 0xB8000;
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0x0F;

static inline uint8_t inb(uint16_t port);
static void terminal_putchar(char c);

static uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

/* Forward declarations for desktop and shell features */
static void file_explorer(void);
static void settings_panel(void);
static void web_browser(void);
static void media_player(void);
static void coco_assistant(void);
static void task_manager(void);
static void mail_calendar(void);
static void edge_browser(void);
static void desktop_status(const char* text);
static void draw_desktop(void);

static void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!framebuffer || x >= fb_width || y >= fb_height) return;
    uint8_t* pixel = (uint8_t*)(framebuffer + y * fb_pitch + x * (fb_bpp / 8));
    if (fb_bpp == 32) {
        *(uint32_t*)pixel = color;
    } else if (fb_bpp == 24) {
        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
    }
}

static void fb_fill(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_put_pixel(x, y, color);
        }
    }
}

static void fb_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t yy = y; yy < y + h && yy < fb_height; yy++) {
        for (uint32_t xx = x; xx < x + w && xx < fb_width; xx++) {
            fb_put_pixel(xx, yy, color);
        }
    }
}

static const uint8_t font5x7[] = {
    /* space */ 0x00,0x00,0x00,0x00,0x00,
    /* A */ 0x7C,0x12,0x11,0x12,0x7C,
    /* B */ 0x7F,0x49,0x49,0x49,0x36,
    /* C */ 0x3E,0x41,0x41,0x41,0x22,
    /* D */ 0x7F,0x41,0x41,0x22,0x1C,
    /* E */ 0x7F,0x49,0x49,0x49,0x41,
    /* F */ 0x7F,0x09,0x09,0x09,0x01,
    /* G */ 0x3E,0x41,0x49,0x49,0x7A,
    /* H */ 0x7F,0x08,0x08,0x08,0x7F,
    /* I */ 0x00,0x41,0x7F,0x41,0x00,
    /* J */ 0x20,0x40,0x41,0x3F,0x01,
    /* K */ 0x7F,0x08,0x14,0x22,0x41,
    /* L */ 0x7F,0x40,0x40,0x40,0x40,
    /* M */ 0x7F,0x02,0x04,0x02,0x7F,
    /* N */ 0x7F,0x04,0x08,0x10,0x7F,
    /* O */ 0x3E,0x41,0x41,0x41,0x3E,
    /* P */ 0x7F,0x09,0x09,0x09,0x06,
    /* Q */ 0x3E,0x41,0x51,0x21,0x5E,
    /* R */ 0x7F,0x09,0x19,0x29,0x46,
    /* S */ 0x46,0x49,0x49,0x49,0x31,
    /* T */ 0x01,0x01,0x7F,0x01,0x01,
    /* U */ 0x3F,0x40,0x40,0x40,0x3F,
    /* V */ 0x1F,0x20,0x40,0x20,0x1F,
    /* W */ 0x7F,0x20,0x18,0x20,0x7F,
    /* X */ 0x63,0x14,0x08,0x14,0x63,
    /* Y */ 0x07,0x08,0x70,0x08,0x07,
    /* Z */ 0x61,0x51,0x49,0x45,0x43,
    /* 0 */ 0x3E,0x45,0x49,0x51,0x3E,
    /* 1 */ 0x00,0x41,0x7F,0x40,0x00,
    /* 2 */ 0x42,0x61,0x51,0x49,0x46,
    /* 3 */ 0x21,0x41,0x45,0x4B,0x31,
    /* 4 */ 0x18,0x14,0x12,0x7F,0x10,
    /* 5 */ 0x27,0x45,0x45,0x45,0x39,
    /* 6 */ 0x3C,0x4A,0x49,0x49,0x30,
    /* 7 */ 0x01,0x01,0x71,0x09,0x07,
    /* 8 */ 0x36,0x49,0x49,0x49,0x36,
    /* 9 */ 0x06,0x49,0x49,0x29,0x1E,
};

static void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    uint8_t mod = 0;
    const uint8_t *glyph = NULL;
    if (c == ' ') {
        fb_rect(x, y, 6, 8, bg);
        return;
    }
    if (c >= 'A' && c <= 'Z') {
        mod = c - 'A' + 1;
    } else if (c >= 'a' && c <= 'z') {
        mod = c - 'a' + 1;
    } else if (c >= '0' && c <= '9') {
        mod = 27 + (c - '0');
    } else {
        fb_rect(x, y, 6, 8, bg);
        return;
    }
    glyph = &font5x7[(mod - 1) * 5];
    fb_rect(x, y, 6, 8, bg);
    for (uint32_t i = 0; i < 5; i++) {
        uint8_t bits = glyph[i];
        for (uint32_t j = 0; j < 7; j++) {
            if (bits & (1 << j)) {
                fb_put_pixel(x + i, y + j, fg);
            }
        }
    }
}

static void fb_draw_text(uint32_t x, uint32_t y, const char* str, uint32_t fg, uint32_t bg) {
    while (*str) {
        fb_draw_char(x, y, *str++, fg, bg);
        x += 6;
    }
}



static char scan_code_to_ascii(uint8_t sc) {
    static const char map[59] = {
        0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\n', 0,
        '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0
    };
    if (sc < sizeof(map)) {
        return map[sc];
    }
    return 0;
}

static char read_char(void) {
    while (1) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            if (scancode < 0x80) {
                char c = scan_code_to_ascii(scancode);
                if (c) return c;
            }
        }
    }
}

static void readline(char* buf, size_t max) {
    size_t i = 0;
    while (1) {
        char c = read_char();
        if (c == '\r' || c == '\n') {
            terminal_putchar('\n');
            buf[i] = '\0';
            return;
        }
        if (c == 0x08 || c == 0x7F) {
            if (i > 0) {
                i--;
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
            continue;
        }
        if (i < max - 1) {
            buf[i++] = c;
            terminal_putchar(c);
        }
    }
}

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * 80 + x;
    VGA_BUFFER[index] = vga_entry(c, color);
}

static void terminal_scroll(void) {
    for (size_t y = 1; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            VGA_BUFFER[(y - 1) * 80 + x] = VGA_BUFFER[y * 80 + x];
        }
    }
    for (size_t x = 0; x < 80; x++) {
        terminal_putentryat(' ', terminal_color, x, 24);
    }
}

static void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == 25) {
            terminal_scroll();
            terminal_row = 24;
        }
        return;
    }
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == 80) {
        terminal_column = 0;
        terminal_row++;
    }
    if (terminal_row == 25) {
        terminal_scroll();
        terminal_row = 24;
    }
}

static void terminal_writestring(const char* str) {
    for (const char* c = str; *c != '\0'; c++) {
        terminal_putchar(*c);
    }
}

static void terminal_putdec(unsigned int value) {
    char buf[11];
    int i = 10;
    buf[i--] = '\0';
    if (value == 0) {
        terminal_putchar('0');
        return;
    }
    while (value > 0 && i >= 0) {
        buf[i--] = '0' + (value % 10);
        value /= 10;
    }
    terminal_writestring(buf + i + 1);
}

static void show_welcome(void) {
    terminal_writestring("=== Coco OS 1.0 ===\n");
    terminal_writestring("Your modern experimental OS with Coco Assistant.\n\n");
}

static void coco_assistant(void) {
    terminal_writestring("Hi, I am Coco (digital assistant).\n");
    terminal_writestring("Ask me for help, open appstore, file explorer, or settings.\n\n");
}

static void draw_desktop(void) {
    if (!gui_mode) return;
    fb_fill(0x00334A); // background
    fb_rect(0, 0, fb_width, 24, 0x00447A); // top bar
    fb_draw_text(8, 8, "Coco OS Desktop", 0xFFFFFF, 0x00447A);
    fb_rect(10, 40, 100, 100, 0x6699CC); // icon 1
    fb_draw_text(20, 80, "Files", 0x000000, 0x6699CC);
    fb_rect(130, 40, 100, 100, 0x66CC99); // icon 2
    fb_draw_text(140, 80, "Web", 0x000000, 0x66CC99);
    fb_rect(250, 40, 100, 100, 0xCC9966); // icon 3
    fb_draw_text(260, 80, "Apps", 0x000000, 0xCC9966);
    fb_rect(0, fb_height - 24, fb_width, 24, 0x005577); // taskbar
    fb_draw_text(8, fb_height - 18, "Start | Feedback | AI (Coco)", 0xFFFFFF, 0x005577);
}

static uint32_t mouse_x = 30;
static uint32_t mouse_y = 50;
static int start_menu_open = 0;

static void draw_mouse(void) {
    if (!gui_mode) return;
    const uint32_t cursor = 0xFFFFFF;
    fb_rect(mouse_x, mouse_y, 10, 2, cursor);
    fb_rect(mouse_x, mouse_y, 2, 10, cursor);
}

static void draw_start_menu(void) {
    if (!gui_mode) return;
    fb_rect(10, 30, 220, 260, 0x223344);
    fb_rect(10, 30, 220, 24, 0x446688);
    fb_draw_text(16, 36, "Start Menu", 0xFFFFFF, 0x446688);
    fb_draw_text(16, 62, "1. File Explorer", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 78, "2. Settings", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 94, "3. Microsoft Edge", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 110, "4. Media Player", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 126, "5. Mail & Calendar", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 142, "6. Task Manager", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 158, "Q. Close Start Menu", 0xFFFFFF, 0x223344);
    fb_draw_text(16, 186, "Use arrow keys / WASD for pointer.", 0xAAFFAA, 0x223344);
    fb_draw_text(16, 202, "Space = Select, ESC = Back", 0xAAFFAA, 0x223344);
}

static void desktop_status(const char* text) {
    if (!gui_mode) return;
    fb_rect(10, fb_height - 40, fb_width - 20, 16, 0x000000);
    fb_draw_text(12, fb_height - 38, text, 0xFFFFFF, 0x000000);
}

static void desktop_loop(void) {
    draw_desktop();
    draw_mouse();
    desktop_status("H=Help E=Explorer A=Assistant S=Settings O=Start Q=Quit");
    while (1) {
        char c = read_char();

        if (c == 'h' || c == 'H') {
            desktop_status("Help: H, E, A, S, W, M, O, Q");
            continue;
        }
        if (c == 'o' || c == 'O') {
            start_menu_open = 1;
            while (start_menu_open) {
                draw_desktop();
                draw_start_menu();
                draw_mouse();
                char s = read_char();
                if (s == '1') { draw_desktop(); file_explorer(); }
                else if (s == '2') { draw_desktop(); settings_panel(); }
                else if (s == '3') { draw_desktop(); edge_browser(); }
                else if (s == '4') { draw_desktop(); media_player(); }
                else if (s == '5') { draw_desktop(); mail_calendar(); }
                else if (s == '6') { draw_desktop(); task_manager(); }
                else if (s == 'q' || s == 'Q' || s == 27) {
                    start_menu_open = 0;
                    draw_desktop();
                }
            }
            desktop_status("Start menu closed.");
            continue;
        }

        if (c == 'e' || c == 'E') {
            desktop_status("Opening File Explorer...");
            file_explorer();
            draw_desktop();
            draw_mouse();
            continue;
        }
        if (c == 'a' || c == 'A') {
            desktop_status("Coco Assistant active...");
            coco_assistant();
            draw_desktop();
            draw_mouse();
            continue;
        }
        if (c == 's' || c == 'S') {
            desktop_status("Opening Settings...");
            settings_panel();
            draw_desktop();
            draw_mouse();
            continue;
        }
        if (c == 'w' || c == 'W') {
            desktop_status("Opening browser...");
            web_browser();
            draw_desktop();
            draw_mouse();
            continue;
        }
        if (c == 'm' || c == 'M') {
            desktop_status("Opening Media Player...");
            media_player();
            draw_desktop();
            draw_mouse();
            continue;
        }

        // Simple pointer / mouse via arrow keys and WASD
        if (c == 'A' || c == 'a' || c == 0x1B) {
            if (mouse_x > 0) mouse_x -= 5;
            draw_desktop(); draw_mouse(); continue;
        }
        if (c == 'D' || c == 'd') {
            if (mouse_x + 10 < fb_width) mouse_x += 5;
            draw_desktop(); draw_mouse(); continue;
        }
        if (c == 'W' || c == 'w') {
            if (mouse_y > 0) mouse_y -= 5;
            draw_desktop(); draw_mouse(); continue;
        }
        if (c == 'S' || c == 's') {
            if (mouse_y + 10 < fb_height) mouse_y += 5;
            draw_desktop(); draw_mouse(); continue;
        }

        if (c == 'q' || c == 'Q') {
            desktop_status("Exiting Desktop, going to shell mode...");
            break;
        }
    }
}

static void app_store(void) {
    terminal_writestring("[App Store] Available apps:\n");
    terminal_writestring("1. Notes (beta)\n");
    terminal_writestring("2. Music (beta)\n");
    terminal_writestring("3. Paint (alpha)\n");
    terminal_writestring("4. Terminal (core)\n");
    terminal_writestring("5. Web Browser (alpha)\n");
    terminal_writestring("6. Calculator (alpha)\n");
    terminal_writestring("7. Chat (alpha)\n");
    terminal_writestring("8. Screenshot Tool (alpha)\n");
    terminal_writestring("9. Windows Terminal (alpha)\n");
    terminal_writestring("10. Microsoft Store (alpha)\n");
    terminal_writestring("11. Microsoft Edge (alpha)\n");
    terminal_writestring("12. Outlook (alpha)\n");
    terminal_writestring("13. Teams (alpha)\n");
    terminal_writestring("14. Phone Link (alpha)\n");
    terminal_writestring("15. OneNote (alpha)\n");
    terminal_writestring("16. Clock (alpha)\n");
    terminal_writestring("17. Photos (alpha)\n");
    terminal_writestring("18. Clipchamp (alpha)\n");
    terminal_writestring("19. Notepad (alpha)\n");
    terminal_writestring("20. Security & Copilot (alpha)\n");
    terminal_writestring("21. Sticky Notes (alpha)\n");
    terminal_writestring("22. Voice Recorder (alpha)\n");
    terminal_writestring("23. Maps (alpha)\n");
    terminal_writestring("24. Weather (alpha)\n");
    terminal_writestring("25. Xbox App (alpha)\n");
    terminal_writestring("26. Solitaire (alpha)\n");
    terminal_writestring("27. YouTube (alpha)\n");
    terminal_writestring("Type 'install <number>' to simulate installation.\n\n");
}

static void file_explorer(void) {
    terminal_writestring("[File Explorer] /coco\n");
    terminal_writestring(" - config.sys\n");
    terminal_writestring(" - wallpapers/\n");
    terminal_writestring(" - apps/\n");
    terminal_writestring(" - Users/\n");
    terminal_writestring("[Feature] Add open/delete/rename for files soon.\n\n");
}

static void task_manager(void) {
    terminal_writestring("[Task Manager]\n");
    terminal_writestring("PID  NAME          CPU%  STATUS\n");
    terminal_writestring("1    coco-shell    2.5   Running\n");
    terminal_writestring("2    mind-assist   0.1   Idle\n");
    terminal_writestring("3    fileexplorer  0.9   Sleeping\n");
    terminal_writestring("Use 'kill <PID>' to force quit in the future.\n\n");
}

static void settings_panel(void) {
    terminal_writestring("[Settings]\n");
    terminal_writestring("1. Update Coco OS\n");
    terminal_writestring("2. Change Wallpaper\n");
    terminal_writestring("3. System Info\n");
    terminal_writestring("4. Network (Wi-Fi, IP)\n");
    terminal_writestring("5. Users & Accounts\n");
    terminal_writestring("Type numbers quickly to test behavior.\n\n");
}

static void web_browser(void) {
    terminal_writestring("[Web Browser] CocoNet v0.1\n");
    terminal_writestring("HTTP request simulation active; no real network this build.\n\n");
}

static void text_editor(void) {
    terminal_writestring("[Text Editor] CocoNote\n");
    terminal_writestring("Create/edit text files with a simple command line style.\n\n");
}

static void calculator(void) {
    terminal_writestring("[Calculator] basic/scientific/programmer modes\n");
    terminal_writestring("Enter 2+2 in future for on-screenwyn answers.\n\n");
}

static void media_player(void) {
    terminal_writestring("[Media Player] CocoPlay\n");
    terminal_writestring("Supports music/video playlist in a later update.\n\n");
}

static void photo_viewer(void) {
    terminal_writestring("[Photo Viewer] CocoFrame\n");
    terminal_writestring("Fast image scrolling + rotate/crop planned.\n\n");
}

static void mail_calendar(void) {
    terminal_writestring("[Mail + Calendar]\n");
    terminal_writestring("Sync email and events (stub).\n\n");
}

static void chat_tool(void) {
    terminal_writestring("[Chat] CocoChat\n");
    terminal_writestring("In-app messaging and calls for teams/collab (stub).\n\n");
}

static void windows_terminal(void) {
    terminal_writestring("[Windows Terminal] CocoTerm\n");
    terminal_writestring("Tabs, shells, and advanced terminal capabilities in development.\n\n");
}

static void microsoft_store(void) {
    terminal_writestring("[Microsoft Store] Coco Store\n");
    terminal_writestring("Install and update apps from the built-in catalog.\n\n");
}

static void edge_browser(void) {
    terminal_writestring("[Microsoft Edge] Coco Edge\n");
    terminal_writestring("Modern web browser with web apps support.\n\n");
}

static void outlook_app(void) {
    terminal_writestring("[Outlook] Coco Mail & Calendar\n");
    terminal_writestring("Email, contacts, and calendar sync (stub).\n\n");
}

static void teams_app(void) {
    terminal_writestring("[Teams] Coco Teams\n");
    terminal_writestring("Chat/video collaboration features stub.\n\n");
}

static void phone_link_app(void) {
    terminal_writestring("[Phone Link] Coco Connect\n");
    terminal_writestring("Connect Android/iPhone to PC (stub).\n\n");
}

static void one_note_app(void) {
    terminal_writestring("[OneNote] Coco Notes\n");
    terminal_writestring("Digital note-taking with notebooks.\n\n");
}

static void clock_app(void) {
    terminal_writestring("[Clock] Coco Alarm\n");
    terminal_writestring("Alarms, timers, stopwatch, focus sessions.\n\n");
}

static void paint_app(void) {
    terminal_writestring("[Paint] Coco Paint\n");
    terminal_writestring("Quick drawing and AI layer support planned.\n\n");
}

static void clipchamp_app(void) {
    terminal_writestring("[Clipchamp] Coco Video Studio\n");
    terminal_writestring("Built-in video editing tools.\n\n");
}

static void notepad_app(void) {
    terminal_writestring("[Notepad] Coco NotePad\n");
    terminal_writestring("Fast plain-text editor for quick edits.\n\n");
}

static void snipping_tool(void) {
    terminal_writestring("[Snipping Tool] CocoClip\n");
    terminal_writestring("Screenshot and recording utility (simulation).\n\n");
}

static void security_center(void) {
    terminal_writestring("[Security] Coco Defender & Copilot\n");
    terminal_writestring("AV, firewall, and AI assistant security insights.\n\n");
}

static void sticky_notes(void) {
    terminal_writestring("[Sticky Notes] Coco Post-it\n");
    terminal_writestring("Quick sticky notes synced to cloud (stub).\n\n");
}

static void voice_recorder(void) {
    terminal_writestring("[Voice Recorder] Coco Recorder\n");
    terminal_writestring("Simple audio recording and playback.\n\n");
}

static void maps_app(void) {
    terminal_writestring("[Maps] Coco Maps\n");
    terminal_writestring("Offline-capable maps and routes.\n\n");
}

static void weather_app(void) {
    terminal_writestring("[Weather] Coco Weather\n");
    terminal_writestring("Real-time weather forecasts and news.\n\n");
}

static void xbox_app(void) {
    terminal_writestring("[Xbox] Coco Games Hub\n");
    terminal_writestring("Game Pass, social, and performance dashboard.\n\n");
}

static void solitaire_app(void) {
    terminal_writestring("[Solitaire] Coco Cards\n");
    terminal_writestring("Classic card games suite.\n\n");
}

static void youtube_app(void) {
    terminal_writestring("[YouTube] CocoTube\n");
    terminal_writestring("Video streaming experience (stub).\n\n");
}

static int starts_with(const char* str, const char* pre) {
    for (size_t i = 0; pre[i] != '\0'; i++) {
        if (str[i] != pre[i]) return 0;
    }
    return 1;
}

static void show_help(void) {
    terminal_writestring("commands:\n");
    terminal_writestring("help - show commands\n");
    terminal_writestring("assistant - interact with Coco assistant\n");
    terminal_writestring("appstore - open app store\n");
    terminal_writestring("explorer - open file explorer\n");
    terminal_writestring("taskmgr - show task manager\n");
    terminal_writestring("settings - open settings\n");
    terminal_writestring("terminal - open command prompt shell\n");
    terminal_writestring("windows-terminal - modern terminal shell\n");
    terminal_writestring("store - open Microsoft Store stub\n");
    terminal_writestring("edge - open Microsoft Edge stub\n");
    terminal_writestring("outlook - open Outlook stub\n");
    terminal_writestring("teams - open Teams stub\n");
    terminal_writestring("phonelink - open Phone Link stub\n");
    terminal_writestring("onenote - open OneNote stub\n");
    terminal_writestring("text - open Notepad/text editor stub\n");
    terminal_writestring("clock - open Clock stub\n");
    terminal_writestring("media - open media player stub\n");
    terminal_writestring("photos - open Photos viewer stub\n");
    terminal_writestring("paint - open Paint editor stub\n");
    terminal_writestring("clipchamp - open Clipchamp editor stub\n");
    terminal_writestring("security - open Windows Security/Copilot stub\n");
    terminal_writestring("sticky - open Sticky Notes stub\n");
    terminal_writestring("voice - open Voice Recorder stub\n");
    terminal_writestring("maps - open Maps stub\n");
    terminal_writestring("weather - open Weather stub\n");
    terminal_writestring("xbox - open Xbox app stub\n");
    terminal_writestring("solitaire - open Solitaire stub\n");
    terminal_writestring("youtube - open YouTube stub\n");
    terminal_writestring("clear - clear screen\n");
    terminal_writestring("reboot - reset system (loop)\n\n");
}

static void shell_loop(void) {
    char line[80];
    while (1) {
        terminal_writestring("coco> ");
        readline(line, sizeof(line));
        if (line[0] == '\0') continue;

        if (starts_with(line, "help")) {
            show_help();
            continue;
        }
        if (starts_with(line, "assistant")) {
            coco_assistant();
            continue;
        }
        if (starts_with(line, "appstore")) {
            app_store();
            continue;
        }
        if (starts_with(line, "explorer")) {
            file_explorer();
            continue;
        }
        if (starts_with(line, "taskmgr")) {
            task_manager();
            continue;
        }
        if (starts_with(line, "settings")) {
            settings_panel();
            continue;
        }
        if (starts_with(line, "web")) {
            web_browser();
            continue;
        }
        if (starts_with(line, "text")) {
            text_editor();
            continue;
        }
        if (starts_with(line, "calc")) {
            calculator();
            continue;
        }
        if (starts_with(line, "media")) {
            media_player();
            continue;
        }
        if (starts_with(line, "photo")) {
            photo_viewer();
            continue;
        }
        if (starts_with(line, "mail")) {
            mail_calendar();
            continue;
        }
        if (starts_with(line, "chat")) {
            chat_tool();
            continue;
        }
        if (starts_with(line, "snip")) {
            snipping_tool();
            continue;
        }
        if (starts_with(line, "terminal")) {
            windows_terminal();
            continue;
        }
        if (starts_with(line, "windows-terminal")) {
            windows_terminal();
            continue;
        }
        if (starts_with(line, "store")) {
            microsoft_store();
            continue;
        }
        if (starts_with(line, "edge")) {
            edge_browser();
            continue;
        }
        if (starts_with(line, "outlook")) {
            outlook_app();
            continue;
        }
        if (starts_with(line, "teams")) {
            teams_app();
            continue;
        }
        if (starts_with(line, "phonelink")) {
            phone_link_app();
            continue;
        }
        if (starts_with(line, "onenote")) {
            one_note_app();
            continue;
        }
        if (starts_with(line, "clock")) {
            clock_app();
            continue;
        }
        if (starts_with(line, "paint")) {
            paint_app();
            continue;
        }
        if (starts_with(line, "clipchamp")) {
            clipchamp_app();
            continue;
        }
        if (starts_with(line, "notepad")) {
            notepad_app();
            continue;
        }
        if (starts_with(line, "security")) {
            security_center();
            continue;
        }
        if (starts_with(line, "copilot")) {
            coco_assistant();
            continue;
        }
        if (starts_with(line, "sticky")) {
            sticky_notes();
            continue;
        }
        if (starts_with(line, "voice")) {
            voice_recorder();
            continue;
        }
        if (starts_with(line, "maps")) {
            maps_app();
            continue;
        }
        if (starts_with(line, "weather")) {
            weather_app();
            continue;
        }
        if (starts_with(line, "xbox")) {
            xbox_app();
            continue;
        }
        if (starts_with(line, "solitaire")) {
            solitaire_app();
            continue;
        }
        if (starts_with(line, "youtube")) {
            youtube_app();
            continue;
        }
        if (line[0] == 'c' && line[1] == 'l' && line[2] == 'e' && line[3] == 'a' && line[4] == 'r') {
            for (size_t y = 0; y < 25; y++) {
                for (size_t x = 0; x < 80; x++) {
                    terminal_putentryat(' ', terminal_color, x, y);
                }
            }
            terminal_row = 0;
            terminal_column = 0;
            continue;
        }
        if (line[0] == 't' && line[1] == 'e' && line[2] == 'r' && line[3] == 'm') {
            terminal_writestring("[Terminal] You are already in a terminal.\n\n");
            continue;
        }
        if (line[0] == 't' && line[1] == 'a' && line[2] == 's' && line[3] == 'k') {
            task_manager();
            continue;
        }
        if (line[0] == 'w' && line[1] == 'e' && line[2] == 'b') {
            web_browser();
            continue;
        }
        if (line[0] == 't' && line[1] == 'e' && line[2] == 'x') {
            text_editor();
            continue;
        }
        if (line[0] == 'c' && line[1] == 'a' && line[2] == 'l' && line[3] == 'c') {
            calculator();
            continue;
        }
        if (line[0] == 'm' && line[1] == 'e' && line[2] == 'd') {
            media_player();
            continue;
        }
        if (line[0] == 'p' && line[1] == 'h' && line[2] == 'o') {
            photo_viewer();
            continue;
        }
        if (line[0] == 'm' && line[1] == 'a' && line[2] == 'i') {
            mail_calendar();
            continue;
        }
        if (line[0] == 'c' && line[1] == 'h' && line[2] == 'a') {
            chat_tool();
            continue;
        }
        if (line[0] == 's' && line[1] == 'n' && line[2] == 'i') {
            snipping_tool();
            continue;
        }
        if (line[0] == 'r' && line[1] == 'e' && line[2] == 'b' && line[3] == 'o' && line[4] == 'o' && line[5] == 't') {
            terminal_writestring("Rebooting...\n");
            terminal_row = 0;
            terminal_column = 0;
            terminal_writestring("Done.\n");
            continue;
        }
        terminal_writestring("Unknown command. Type help.\n");
    }
}

void kernel_main(uint32_t magic, uint32_t addr) {
    if (magic == MULTIBOOT_MAGIC) {
        multiboot_info_t* mb = (multiboot_info_t*) addr;
        if (mb->flags & (1 << 12)) {
            framebuffer = (volatile uint8_t*)(uintptr_t) mb->framebuffer_addr;
            fb_width = mb->framebuffer_width;
            fb_height = mb->framebuffer_height;
            fb_pitch = mb->framebuffer_pitch;
            fb_bpp = mb->framebuffer_bpp;
            gui_mode = 1;
        }
    }

    if (gui_mode) {
        draw_desktop();
        desktop_loop();
        // fall back to text shell after desktop exit
    }

    terminal_color = vga_color(15, 1);
    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    terminal_row = 0;
    terminal_column = 0;

    show_welcome();
    coco_assistant();
    show_help();
    shell_loop();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
