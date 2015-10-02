/*
 * bbHelper.cpp
 *
 *  Created on: Jan 8, 2015
 *      Author: ULUMU
 */

#include <errno.h>
#include <iosfwd>
#include <stdio.h>
#include <sys/stat.h>
#include <fstream>
#include "bbDialog.h"

extern void UpdateRomList(char *filename);

static bool check_mkdir( const char *path, mode_t mode )
{
    struct stat64 st;
    if(stat64(path, &st) == 0)
    {
        if( (S_ISDIR(st.st_mode)) )
        {
            return true;
        }
    }

    if ( (mkdir(path, mode) != 0) && (errno != EEXIST) )
    {
        return false;
    }

    return true;
}

int createDefaultConfig()
{
    /*
     * Create GBA folder for configuration and ROMS
     */
    if (false == check_mkdir("/accounts/1000/shared/misc/roms",0777) )              return -1;
    if (false == check_mkdir("/accounts/1000/shared/misc/gbaemu", 0777) )           return -1;
    if (false == check_mkdir("/accounts/1000/shared/misc/gbaemu/savegames", 0777) ) return -1;
    if (false == check_mkdir("/accounts/1000/shared/misc/roms/gba",0777) )          return -1;

    /*
     * If config file is not is misc/gbaemu, copy the default one
     */
    ifstream ifile2("/accounts/1000/shared/misc/gbaemu/gpsp.cfg");
    if(!ifile2){
        ifstream f11("app/native/gpsp.cfg", fstream::binary);
        ofstream f22("/accounts/1000/shared/misc/gbaemu/gpsp.cfg", fstream::trunc|fstream::binary);
        f22 << f11.rdbuf();
        f11.close();
        f22.close();
    } else {
        ifile2.close();
    }

    /*
     * Copy game_config.txt to savegames folder if not there
     */
    ifstream ifile3("/accounts/1000/shared/misc/gbaemu/savegames/game_config.txt");
    if(!ifile3){
        ifstream f11("app/native/game_config.txt", fstream::binary);
        ofstream f22("/accounts/1000/shared/misc/gbaemu/savegames/game_config.txt", fstream::trunc|fstream::binary);
        f22 << f11.rdbuf();
        f11.close();
        f22.close();
    } else {
        ifile3.close();
    }

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

extern void sdlReadPreferences(FILE *f);

/*
 * Helper function interface with C program
 */
void bbShowAlert(const char *title, const char *content)
{
	bbDialog dialog;

	dialog.showAlert(title, content);
}


void bbShowNotification(const char *content)
{
	bbDialog dialog;

	dialog.showNotification(content);
}

void loadRomDialog(char *filename)
{
	UpdateRomList(filename);
}


void loadConfiguration(void)
{
    createDefaultConfig();

    FILE *cfgFile = fopen("/accounts/1000/shared/misc/gbaemu/gpsp.cfg", "r");
    if (cfgFile) {
        sdlReadPreferences(cfgFile);
        fclose(cfgFile);
    }
}

#ifdef __cplusplus
}
#endif


