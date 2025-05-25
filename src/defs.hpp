#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOVE_SHIP 'm'
#define ADD_SHIP 'a'
#define PASTE_SHIP 'p'
#define ADD_BOMB 'b'
#define MOVE_BOMB 'B'
#define PASTE_BOMB 'P'
#define MOVE_1_BUTTONX 12
#define MOVE_1_BUTTONY 13
#define MOVE_1_BUTTON_ORIENTATION 15
#define MOVE_1_DROP_SHIP 17
#define PLAYER_LED 33
#define INVALID_LED 27
#define PIXEL_MULTIPLIER 4
#define SHIP_NR 8
#define BOMB_NR 8
#define SHIP_HEIGHT PIXEL_MULTIPLIER * 5 * 2
#define SHIP_WIDTH PIXEL_MULTIPLIER * 2

#define BOMB_HEIGHT PIXEL_MULTIPLIER * 2
#define BOMB_WIDTH PIXEL_MULTIPLIER * 2
#define PUTTING_SHIPS 0
#define PUTTING_BOMBS 1

struct ParsedCommand {
    char type;       
    int pos1;
    int pos2;
    int orientation;  // only for ships, -1 if not used
};

int parse_command(const char* input, ParsedCommand* out) {
    char buffer[100];
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* token = strtok(buffer, "|");
    if (!token) return -1;

    out->type = token[0];

    // Parse pos1
    token = strtok(NULL, "|");
    if (!token) return -1;
    out->pos1 = atoi(token);

    // Parse pos2
    token = strtok(NULL, "|");
    if (!token) return -1;
    out->pos2 = atoi(token);

    // Parse orientation (for ships only)
    if (1) {//(out->type == ADD_SHIP || out->type == MOVE_SHIP || out->type == PASTE_SHIP || out->type == MOVE_BOMB) {
        token = strtok(NULL, "|");
        if (!token) return -1;
        out->orientation = atoi(token);
    } else {
        out->orientation = -1;  // Not applicable
    }

    return 0;  // Success
}

struct Ship {
	int16_t pos1 = -1;
	int16_t pos2 = 0;
	int16_t orientation = 0;
};


struct Bomb {
	int16_t pos1 = -1;
	int16_t pos2 = 0;
	int16_t orientation = 0;
};
