#ifndef DEVCONSOLE_ANSI_H
#define DEVCONSOLE_ANSI_H

#include <stdint.h>

#define ANSI_CURSOR_UP          'A'
#define ANSI_CURSOR_DOWN        'B'
#define ANSI_CURSOR_FORWARD     'C'
#define ANSI_CURSOR_BACK        'D'
#define ANSI_CURSOR_NEXT_LINE   'E'
#define ANSI_CURSOR_PREV_LINE   'F'
#define ANSI_CURSOR_HORZ_ABS    'G'
#define ANSI_CURSOR_POSITION    'H'
#define ANSI_ERASE_IN_DISPLAY   'J'
#define ANSI_ERASE_IN_LINE      'K'
#define ANSI_INSERT_BLANK_LINES 'L'
#define ANSI_DELETE_LINES       'M'
#define ANSI_SCROLL_UP          'S'
#define ANSI_SCROLL_DOWN        'T'
#define ANSI_SCREEN_MODE        'h'
#define ANSI_SCREEN_RESET       'l'
#define ANSI_SCREEN_MARGIN      'r'

#define ANSI_REQUEST_CURSOR_1    'n'
#define ANSI_REQUEST_CURSOR_0    '6'


#define ANSI_PRIVATE                    (char)'?'
#define ANSI_CONTROL                    (char)'['
#define ANSI_OS_COMMAND                 (char)']'
#define ANSI_CHARACTER_SET              (char)'('
#define ANSI_GRAPHICS                   (char)'m'
#define ANSI_SEPARATOR                  (char)';'

#define ASCII_BEL                       7

#define ANSI_CURSOR                     25

#define ANSI_GRAPHICS_RESET              0
#define ANSI_GRAPHICS_BOLD               1
#define ANSI_GRAPHICS_FAINT              2
#define ANSI_GRAPHICS_ITALIC             3
#define ANSI_GRAPHICS_UNDERLINE          4
#define ANSI_GRAPHICS_SLOW_BLINK         5
#define ANSI_GRAPHICS_RAPID_BLINK        6
#define ANSI_GRAPHICS_REVERSE_VIDEO      7
#define ANSI_GRAPHICS_CONCEAL            8
#define ANSI_GRAPHICS_STRIKETHROUGH      9
#define ANSI_GRAPHICS_FONT_PRIMARY       10
#define ANSI_GRAPHICS_FONT_ALTERNATE     11
#define ANSI_GRAPHICS_DOUBLE_UNDERLINE   21
#define ANSI_GRAPHICS_NORMAL_INTENSITY   22
#define ANSI_GRAPHICS_UNDERLINE_OFF      24
#define ANSI_GRAPHICS_BLINK_OFF          25
#define ANSI_GRAPHICS_REVERSE_VIDEO_OFF  27

#define ANSI_GRAPHICS_FG_BLACK           30
#define ANSI_GRAPHICS_FG_RED             31
#define ANSI_GRAPHICS_FG_GREEN           32
#define ANSI_GRAPHICS_FG_BROWN           33
#define ANSI_GRAPHICS_FG_BLUE            34
#define ANSI_GRAPHICS_FG_MAGENTA         35
#define ANSI_GRAPHICS_FG_CYAN            36
#define ANSI_GRAPHICS_FG_WHITE           37
#define ANSI_GRAPHICS_FG_COLOR           38
#define ANSI_GRAPHICS_FG_DEFAULT         39

#define ANSI_GRAPHICS_BG_BLACK           40
#define ANSI_GRAPHICS_BG_RED             41
#define ANSI_GRAPHICS_BG_GREEN           42
#define ANSI_GRAPHICS_BG_BROWN           43
#define ANSI_GRAPHICS_BG_BLUE            44
#define ANSI_GRAPHICS_BG_MAGENTA         45
#define ANSI_GRAPHICS_BG_CYAN            46
#define ANSI_GRAPHICS_BG_WHITE           47
#define ANSI_GRAPHICS_BG                 48
#define ANSI_GRAPHICS_BG_DEFAULT         49

#define ANSI_GRAPHICS_FG_BLACK_BRIGHT    90
#define ANSI_GRAPHICS_FG_RED_BRIGHT      91
#define ANSI_GRAPHICS_FG_GREEN_BRIGHT    92
#define ANSI_GRAPHICS_FG_BROWN_BRIGHT    93
#define ANSI_GRAPHICS_FG_BLUE_BRIGHT     94
#define ANSI_GRAPHICS_FG_MAGENTA_BRIGHT  95
#define ANSI_GRAPHICS_FG_CYAN_BRIGHT     96
#define ANSI_GRAPHICS_FG_WHITE_BRIGHT    97

#define ANSI_GRAPHICS_BG_BLACK_BRIGHT    100
#define ANSI_GRAPHICS_BG_RED_BRIGHT      101
#define ANSI_GRAPHICS_BG_GREEN_BRIGHT    102
#define ANSI_GRAPHICS_BG_BROWN_BRIGHT    103
#define ANSI_GRAPHICS_BG_BLUE_BRIGHT     104
#define ANSI_GRAPHICS_BG_MAGENTA_BRIGHT  105
#define ANSI_GRAPHICS_BG_CYAN_BRIGHT     106
#define ANSI_GRAPHICS_BG_WHITE_BRIGHT    107


#endif // ANSI_H