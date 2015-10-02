/*
 * romlisting.cpp
 *
 *  Created on: Jan 8, 2015
 *      Author: ULUMU
 */
#include "bbDialog.h"
#include "../logging.h"
#include <dirent.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <string>
#include <string.h>

//using namespace std;

static std::vector<std::string> romList;
static std::vector<std::string> sortedRomList;

static std::vector<std::string> cheatList;
static std::vector<std::string> sortedCheatList;

#define SYSROMDIR      "/accounts/1000/shared/misc/roms/gba/"
#define SDCARDROMDIR   "/accounts/1000/removable/sdcard/misc/roms/gba/"
#define GBA_VERSION    "1.0.0.2"

std::vector<std::string> sortAlpha(std::vector<std::string> sortThis)
{
	int swap;
	std::string temp;

	do
	{
		swap = 0;
		for (int count = 0; count < (int)sortThis.size() - 1; count++)
		{
			if (sortThis.at(count) > sortThis.at(count + 1))
			{
				temp = sortThis.at(count);
				sortThis.at(count) = sortThis.at(count + 1);
				sortThis.at(count + 1) = temp;
				swap = 1;
			}
		}
	}while (swap != 0);

	return sortThis;
}

void GetRomDirListing(std::vector<std::string> &romList, std::vector<std::string> &cheatList, const char *dpath )
{
	DIR* dirp;
	struct dirent64* direntp;

	if(!dpath)
	{
		SLOG("dpath is null.\n");
		return;
	}

	dirp = opendir( dpath );
	if( dirp != NULL )
	{
		SLOG("[---- Parsing %s ----]", dpath);
		for(;;)
		{
			direntp = readdir64( dirp );
			if( direntp == NULL )
			{
				SLOG("End of readdir64");
				break;
			}

			std::string tmp = direntp->d_name;

			if( strcmp( direntp->d_name, ".") == 0)
			{
				continue;
			}

			if( strcmp( direntp->d_name,"..") == 0)
				continue;

			if( (tmp.substr(tmp.find_last_of(".") + 1) == "gba") ||
				(tmp.substr(tmp.find_last_of(".") + 1) == "GBA")   )
			{
				tmp = dpath;
				tmp += direntp->d_name;
				SLOG("ROM: %s", tmp.c_str());
				romList.push_back(tmp);
			}
			else if(
				(tmp.substr(tmp.find_last_of(".") + 1) == "cht") ||
				(tmp.substr(tmp.find_last_of(".") + 1) == "CHT")   )
			{
				tmp = dpath;
				tmp += direntp->d_name;
				SLOG("CHEAT: %s", tmp.c_str());
				cheatList.push_back(tmp);
			}
		}
		closedir(dirp);
	}
	else
	{
		SLOG("[%s] not found ...", dpath);
	}

}

void UpdateRomList(char *filename)
{
	if(!filename) return;

	filename[0] = '\0';

	romList.clear();
	cheatList.clear();
	sortedRomList.clear();
	sortedCheatList.clear();

	SLOG("Obtain Device ROM list and CHEAT list...");
	GetRomDirListing(romList, cheatList, SYSROMDIR);

	SLOG("Obtain SD-Card ROM list and CHEAT list...");
	GetRomDirListing(romList, cheatList, SDCARDROMDIR);

	SLOG("Total ROMS found: %d", romList.size() );
	SLOG("Total CHEATS found: %d", cheatList.size() );
	if (romList.size() > 0)
	{
		sortedRomList = sortAlpha(romList);

		if (cheatList.size() > 0)
		{
			sortedCheatList = sortAlpha(cheatList);
		}
	}
	else
	{
		bbDialog *dialog = new (bbDialog);

		if (dialog)
		{
			dialog->showAlert("gpsp_bb Error Report", "ERROR: You do not have any ROMS! Add GBA BIOS & ROMS to:\"misc/roms/gba\"");
			delete dialog;
		}

		return;
	}


	const char   *szFile = 0;
	const char  **list = 0;
	int           count = 0;
	std::string        romfilename;
	bbDialog     *dialog = NULL;
	int           gameIndex;

	SLOG("Sorted List: %d", sortedRomList.size() );
	list = (const char**)malloc(sortedRomList.size()*sizeof(char*));

	// ROM selection
	if (list)
	{
		for(count = 0; count < (int)sortedRomList.size(); count++)
		{
			romfilename = sortedRomList.at(count);
			list[count] = sortedRomList[count].c_str() + romfilename.find_last_of("/") + 1;
		}

		SLOG("Creating BB dialog...");
		dialog = new bbDialog;

		if (dialog)
		{
			gameIndex = dialog->showPopuplistDialog(list, sortedRomList.size(), "gpsp_bb [v" GBA_VERSION "]  ROM Selector");
			delete dialog;

			if (gameIndex >= 0)
			{
				romfilename = sortedRomList.at(gameIndex);
				strcpy(filename, romfilename.c_str());
			}
			else
			{
				SLOG("Bad selection index from Popup List Dialog");
			}
		}
		else
		{
			SLOG("Fail creating BB dialog, quiting");
		}

		free(list);
	}
	else
	{
		SLOG("Out of Memory, Fail creating ROM list, quiting!!");
	}

}


