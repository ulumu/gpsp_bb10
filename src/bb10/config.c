/*
 * config.c
 *
 *  Created on: Apr 3, 2015
 *      Author: ULUMU
 */
#include "../common.h"
#include <stdio.h>
#include <string.h>

FILE *fLog = NULL;
int   filterType = 0; // 0:No linear filter, 1:Use linear filter

static u32 sdlFromHex(char *s)
{
    u32 value;
    sscanf(s, "%x", &value);
    return value;
}

static u32 sdlFromDec(char *s)
{
    u32 value = 0;
    sscanf(s, "%u", &value);
    return value;
}

void sdlReadPreferences(FILE *f)
{
    char buffer[2048];

    if(!f)
        return;

    while(1) {
        char *s = fgets(buffer, 2048, f);

        if(s == NULL)
            break;

        char *p  = strchr(s, '#');

        if(p)
            *p = 0;

        char *token = strtok(s, " \t\n\r=");

        if(!token)
            continue;

        if(strlen(token) == 0)
            continue;

        char *key = token;
        char *value = strtok(NULL, "\t\n\r");

        if(value == NULL) {
            SLOG( "Empty value for key %s\n", key);
            continue;
        }

        if(!strcmp(key,"Joy0_Left")) {
            keyboard_config_map[KEY_LEFT] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_Right")) {
            keyboard_config_map[KEY_RIGHT] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_Up")) {
            keyboard_config_map[KEY_UP] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_Down")) {
            keyboard_config_map[KEY_DOWN] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_A")) {
            keyboard_config_map[KEY_BUTTON_A] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_B")) {
            keyboard_config_map[KEY_BUTTON_B] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_L")) {
            keyboard_config_map[KEY_BUTTON_L] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_R")) {
            keyboard_config_map[KEY_BUTTON_R] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_Start")) {
            keyboard_config_map[KEY_BUTTON_START] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy0_Select")) {
            keyboard_config_map[KEY_BUTTON_SELECT] = sdlFromHex(value);
        } else if(!strcmp(key,"Joy1_Left")) {
            gamepad_config_map[KEY_LEFT] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_Right")) {
            gamepad_config_map[KEY_RIGHT] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_Up")) {
            gamepad_config_map[KEY_UP] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_Down")) {
            gamepad_config_map[KEY_DOWN] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_A")) {
            gamepad_config_map[KEY_BUTTON_A] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_B")) {
            gamepad_config_map[KEY_BUTTON_B] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_L")) {
            gamepad_config_map[KEY_BUTTON_L] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_R")) {
            gamepad_config_map[KEY_BUTTON_R] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_Start")) {
            gamepad_config_map[KEY_BUTTON_START] = sdlFromHex(value);
        } else if(!strcmp(key, "Joy1_Select")) {
            gamepad_config_map[KEY_BUTTON_SELECT] = sdlFromHex(value);
        } else if(!strcmp(key, "logToFile")) {
            int flag = sdlFromDec(value);
            if (flag) {
                fLog = fopen("/accounts/1000/shared/misc/gbaemu/gpsplog", "w+");
            }
        } else if(!strcmp(key, "filterType")) {
            int type = sdlFromDec(value);
            if (type >=0 && type < 2) {
                filterType = type;
            } else {
                filterType = 0;
            }
        } else {
            SLOG( "Unknown configuration key %s\n", key);
        }

    }
}
