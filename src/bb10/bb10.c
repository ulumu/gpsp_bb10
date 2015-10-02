/*
 * gpSP - Raspberry Pi port
 *
 * Copyright (C) 2013 DPR <pribyl.email@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../common.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gles_video.h"
#include "time.h"
#include <bps/dialog.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <SDL.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "../logging.h"

// <--------------------- All Memory block declaration ------------------>

#define GBA_ROMCACHE_ADDR (0x08400000)

#if 0
u16  *palette_ram;
u16  *palette_ram_converted;
u16  *oam_ram;
u16  *io_registers;         // io_registers is 1K, why need 16K? [1024 * 16];
u8   *ewram;                // ewram is 256K with mirroring address [1024 * 256 * 2];
u8   *iwram;                // iwram is 32K with mirroring address  [1024 * 32 * 2];
u8   *vram;                 // vram is 128K [1024 * 96 * 2];
u8   *bios_rom;             // bios is 16K [1024 * 32];
u8   *gamepak_rom;
u32  bios_read_protect;
#endif

// <--------------------------------------------------------------------->

// <--------------------- Shared Memory declaration --------------------->
struct _shmNode_t
{
	u8    *memPtr;
	int    memSize;
	int    fd;
	const char  *name;
	struct _shmNode_t *next;
};
typedef struct _shmNode_t shmNode_t;

static shmNode_t *rom_node         = NULL;
static shmNode_t *wram_node        = NULL;
static shmNode_t *iram_node        = NULL;
static shmNode_t *romcache_node    = NULL;
static shmNode_t *ramcache_node    = NULL;
static shmNode_t *bioscache_node   = NULL;


u8 *rom_translation_cache;
u8 *ram_translation_cache;
u8 *bios_translation_cache;
u8 *rom_translation_ptr;
u8 *ram_translation_ptr;
u8 *bios_translation_ptr;
u8 *last_rom_translation_ptr;
u8 *last_ram_translation_ptr;
u8 *last_bios_translation_ptr;

// <--------------------------------------------------------------------->

// <------------------------- mmap allocation macros -------------------->
#if 1
#define GBA_MMAP(type, userptr, memaddr, memsize)                                                                        \
		if ( memaddr != (userptr = (type *)mmap(memaddr, memsize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON, NOFD, 0)) )  {        \
			SLOG("%s virtual memory mapping failed, request:0x%X, got:0x%X", #userptr, (int)memaddr, (int)userptr);      \
			/*return 0;*/                                                                                                    \
		}
#else
#define GBA_MMAP(type, userptr, memaddr, memsize)                                                                        \
		if ( NULL == (userptr = (type *)malloc(memsize)) )  {        \
			SLOG("%s virtual memory mapping failed, request:0x%X, got:0x%X", #userptr, (int)memaddr, (int)userptr);      \
			return 0;                                                                                                    \
		}
#endif

static void gba_addShardMem(shmNode_t *curNode, const char *memName, void *memAddr, int memSize)
{
	int fdint;
	u32 mirror_addr;
	u8 *tmpPtr;

	fdint = shm_open( memName, O_RDWR, 0777 );
	if( fdint == -1 ) {
		signal( SIGSEGV, SIG_DFL );
		return;
	}

	mirror_addr  = ((int)(memAddr) + (memSize-1)) & (~(memSize-1));

	tmpPtr = (u8 *)mmap((void *)(mirror_addr), memSize, PROT_READ|PROT_WRITE, MAP_SHARED, fdint, 0);
	if ((u32)tmpPtr != mirror_addr)
	{
//		munmap(tmpPtr, memSize);
		SLOG("Failed dynamic mapping mirror virtual address 0x%X for WORKRAM, got:0x%X", (int)memAddr, (int)tmpPtr);
//		signal( SIGSEGV, SIG_DFL );
	}
//	else
	{
		shmNode_t *newNode;
		shmNode_t *shmNode = curNode;

		while( shmNode->next ) {
			shmNode = shmNode->next;
		}

		newNode = (shmNode_t *)calloc(1, sizeof(shmNode_t));
		if (newNode)
		{
			newNode->fd      = shmNode->fd;
			newNode->memPtr  = tmpPtr;
			newNode->memSize = shmNode->memSize;
			newNode->name    = shmNode->name;
			shmNode->next    = newNode;
		}
	}

}


static void gba_shmNodeCleanup(shmNode_t *node)
{
	shmNode_t *preNode;

	if (NULL == node) return;

	close(node->fd);
	shm_unlink(node->name);

	while (node)
	{
		munmap(node->memPtr, node->memSize);
		preNode = node;
		node    = node->next;
		free(preNode);
	}
}


static shmNode_t *gba_createSharedMem(const char *memName, void *memAddr, int memSize)
{
	int        fd;
	u8        *memPtr;
	shmNode_t *shmNode;

	shmNode = (shmNode_t *)calloc(1, sizeof(shmNode_t));
	if (NULL == shmNode) {
		return NULL;
	}

    /* Create a new memory object */
	fd = shm_open( memName, O_RDWR | O_CREAT, 0777 );
    if( fd == -1 ) {
        return NULL;
    }

    /* Set the memory object's size */
    if( ftruncate( fd, memSize ) == -1 ) {
        return NULL;
    }

	memPtr = (u8 *)mmap64(memAddr, memSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, 0);
	if(memPtr != memAddr) {
		SLOG("****** Failed to allocate exact Shared memory for %s", memName);
		SLOG("****** Request size=0x%X, addr:0x%X, got:0x%X", memSize, (int)memAddr, (int)memPtr);

//		close(fd);
//		shm_unlink(memName);
//		munmap(memPtr, memSize);
//
//		return NULL;
	}
	memset(memPtr, 0, memSize);

	shmNode->fd      = fd;
	shmNode->memPtr  = memPtr;
	shmNode->memSize = memSize;
	shmNode->name    = memName;
	shmNode->next    = NULL;

	return shmNode;
}

static int gba_memory_virtual_mapping(void)
{
#if 0
	GBA_MMAP(u8, bios_rom, BIOS_ADDR, BIOS_SIZE);

//	rom_node = gba_createSharedMem(ROM_NAME, ROM_ADDR, ROM_SIZE);
//	if (NULL == rom_node) { SLOG("Fail allocating ROM memory"); return 0; }
//	gba_addShardMem(rom_node, ROM_NAME, ROM_ADDR+ROM_SIZE, ROM_SIZE);
	GBA_MMAP(u8, gamepak_rom, ROM_ADDR, ROM_SIZE);

//	wram_node = gba_createSharedMem(WORKRAM_NAME, WORKRAM_ADDR, WORKRAM_SIZE);
//	if (NULL == wram_node) { SLOG("Fail allocating WRAM memory"); return 0; }
//	gba_addShardMem(wram_node, WORKRAM_NAME, WORKRAM_ADDR+WORKRAM_SIZE, WORKRAM_SIZE);
	GBA_MMAP(u8, ewram, WORKRAM_ADDR, WORKRAM_SIZE);

//	iram_node = gba_createSharedMem(INTRAM_NAME, INTRAM_ADDR, INTRAM_SIZE);
//	if (NULL == iram_node) { SLOG("Fail allocating IRAM memory"); return 0; }
//	gba_addShardMem(iram_node, INTRAM_NAME, INTRAM_ADDR+INTRAM_SIZE, INTRAM_SIZE);
	GBA_MMAP(u8, iwram, INTRAM_ADDR, INTRAM_SIZE);

	GBA_MMAP(u16, io_registers,          IOMEM_ADDR,   IOMEM_SIZE);
	GBA_MMAP(u16, palette_ram,           PALETTE_ADDR, PALETTE_SIZE);
	GBA_MMAP(u16, palette_ram_converted, PALCONV_ADDR, PALCONV_SIZE);
	GBA_MMAP(u8,  vram,                  VRAM_ADDR,    VRAM_SIZE);
	GBA_MMAP(u16, oam_ram,               OAM_ADDR,     OAM_SIZE);

//	gamepak_rom = rom_node->memPtr;
//	ewram       = wram_node->memPtr;
//	iwram       = iram_node->memPtr;
#endif
	//** Translation memory allocation for BB10/QNX is handled here as platform
	//** specific. Reason is that memory mapped feature is specific to QNX OS
	//** All mapped memory have to be within the same page between the emulator
	//** and the translation cache location

	/*
	 * ROM cache translation memory mapping
	 */
	int mappedAddr = GBA_ROMCACHE_ADDR;
	romcache_node = gba_createSharedMem("/gbarom_translation_cache", (void *)mappedAddr, ROM_TRANSLATION_CACHE_SIZE);
	if (NULL == romcache_node) {
		SLOG("Fail allocating ROM translation cache");
		return 0;
	}
	SLOG("rom_translation_cache = 0x%.8X", (int)romcache_node->memPtr);
	rom_translation_cache = romcache_node->memPtr;

	/*
	 * BIOS cache translation memory mapping
	 */
	mappedAddr = (int)romcache_node->memPtr + ROM_TRANSLATION_CACHE_SIZE;
	bioscache_node = gba_createSharedMem("/gbabios_translation_cache", (void *)mappedAddr, BIOS_TRANSLATION_CACHE_SIZE);
	if (NULL == bioscache_node) {
		SLOG("Fail allocating BIOS translation cache");
		return 0;
	}
	SLOG("bios_translation_cache = 0x%.8X", (int)bioscache_node->memPtr);
	bios_translation_cache = bioscache_node->memPtr;

	/*
	 * RAM cache translation memory mapping
	 */
	mappedAddr = (int)bioscache_node->memPtr + (BIOS_TRANSLATION_CACHE_SIZE);
	ramcache_node = gba_createSharedMem("/gbaram_translation_cache", (void *)mappedAddr, RAM_TRANSLATION_CACHE_SIZE);
	if (NULL == ramcache_node) {
		SLOG("Fail allocating RAM translation cache");
		return 0;
	}
	SLOG("ram_translation_cache = 0x%.8X", (int)ramcache_node->memPtr);
	ram_translation_cache     = ramcache_node->memPtr;

	rom_translation_ptr       = rom_translation_cache;
	ram_translation_ptr       = ram_translation_cache;
	bios_translation_ptr      = bios_translation_cache;

	last_rom_translation_ptr  = rom_translation_cache;
	last_ram_translation_ptr  = ram_translation_cache;
	last_bios_translation_ptr = bios_translation_cache;

	return 1;
}



//Default setting of gamepad
u32 gamepad_config_map[PLAT_BUTTON_COUNT] =
{
		JOY_ASIX_YM,      // Up
		JOY_ASIX_YP,      // Down
		JOY_ASIX_XM,      // Left
		JOY_ASIX_XP,      // Right
		JOY_BUTTON_1,     // Button A
		JOY_BUTTON_2,     // Button B
		JOY_BUTTON_5,     // Button L
		JOY_BUTTON_6,     // Button R
		JOY_BUTTON_3,     // Button Select
		JOY_BUTTON_4      // Button Start
};

#ifdef BB10_BUILD
//Default setting of keyboard
u32 keyboard_config_map[PLAT_KEY_COUNT] =
{
		0x77,            // Up
		0x73,            // Down
		0x61,            // Left
		0x64,            // Right
		0x6c,            // Button A
		0x6b,            // Button B
		0x71,            // Button L
		0x70,            // Button R
		0x0D,            // Button Select
		0x08             // Button Start
};
#else
u32 keyboard_config_map[PLAT_KEY_COUNT] =
{
		SDLK_UP,          // Up
		SDLK_DOWN,        // Down
		SDLK_LEFT,        // Left
		SDLK_RIGHT,       // Right
		SDLK_z,           // Button A
		SDLK_x,           // Button B
		SDLK_a,           // Button L
		SDLK_s,           // Button R
		SDLK_RETURN,      // Button Select
		SDLK_BACKSPACE    // Button Start
};
#endif



#define MAX_VIDEO_MEM (480*270*4)
static int video_started=0;
#ifdef GBA_USE_RGBA8888
static uint32_t * video_buff;
#else
static uint16_t * video_buff;
#endif

void gpsp_setFullScreen(void)
{
    SDL_Surface* myVideoSurface;

    int flags = SDL_ANYFORMAT | SDL_FULLSCREEN | SDL_OPENGL | SDL_RESIZABLE;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    flags |= SDL_OPENGL | SDL_RESIZABLE;

#ifdef GBA_USE_RGBA8888
    myVideoSurface = SDL_SetVideoMode(480, 320, 32, flags); // STL100_1 device and simulator can only support 32bit surface type
#else
    myVideoSurface = SDL_SetVideoMode(480, 320, 16, flags); // 16 bit surface type for all other devices
#endif

    // Print out some information about the video surface
    if (myVideoSurface == NULL) {
        SLOG( "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
}

void gpsp_setNormalAspectRatio(void)
{
    SDL_Surface* myVideoSurface;

    int flags = SDL_ANYFORMAT | SDL_OPENGL | SDL_RESIZABLE;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    flags |= SDL_OPENGL | SDL_RESIZABLE;

#ifdef GBA_USE_RGBA8888
    myVideoSurface = SDL_SetVideoMode(480, 320, 32, flags); // STL100_1 device and simulator can only support 32bit surface type
#else
    myVideoSurface = SDL_SetVideoMode(480, 320, 16, flags); // 16 bit surface type for all other devices
#endif

    // Print out some information about the video surface
    if (myVideoSurface == NULL) {
        SLOG( "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
}


void gpsp_plat_init(void)
{
	int ret;
	//const char *layer_fb_name;

	bps_initialize();
	dialog_request_events(0);

	if(0 == gba_memory_virtual_mapping()) {
		SLOG("Virtual memory mapping fail, quiting");
		gpsp_plat_quit();
		exit(1);
	}

	ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE );
	if (ret != 0) {
		SLOG( "SDL_Init failed: %s\n", SDL_GetError());
		gpsp_plat_quit();
		exit(1);
	}

	gpsp_setNormalAspectRatio();

	SDL_ShowCursor(0);
	fb_set_mode(240, 160, 0, 0, 0, 0);
	screen_scale = fullscreen;
}

void gpsp_plat_quit(void)
{
	SLOG("Shutdown and cleanup");

	if (video_started) {
		video_close();
		free(video_buff);
		video_started=0;
	}
	SDL_Quit();

	if (romcache_node)  gba_shmNodeCleanup(romcache_node);
	if (ramcache_node)  gba_shmNodeCleanup(ramcache_node);
	if (bioscache_node) gba_shmNodeCleanup(bioscache_node);
	if (rom_node)       gba_shmNodeCleanup(rom_node);
	if (wram_node)      gba_shmNodeCleanup(wram_node);
	if (iram_node)      gba_shmNodeCleanup(iram_node);


	bps_shutdown();
}


void *fb_flip_screen(void)
{
	video_draw(video_buff);
	return video_buff;
}

void fb_wait_vsync(void)
{
    SDL_GL_SwapBuffers();
}

void fb_set_mode(int w, int h, int buffers, int scale,int filter, int filter2)
{
	if (video_started) {
		video_close();
		free(video_buff);
	}
	video_buff = (uint16_t *)malloc(w*h*sizeof(uint32_t));   // Using 32bit Pixel type for RGBA8888 support
	memset(video_buff,0,w*h*sizeof(uint32_t));
	video_init(w,h,filter);
	video_started=1;
}


int get_joystick(void) {
	SDL_Event event;
	int Back=-2;
	long timer;

	timer=time(NULL)+10;
	while(Back==-2) {
		if (time(NULL)>timer) {
			Back=-1;
		}
		if (SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				quit();
				break;

			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE){
					Back=-1;
				};
				break;

			case SDL_JOYBUTTONDOWN:
				if (event.jbutton.button<16) Back=event.jbutton.button+JOY_BUTTON_1;
				break;

			case SDL_JOYAXISMOTION: {
				if (event.jaxis.axis==0) { //Left-Right
					if (event.jaxis.value < -TRESHOLD)  Back = JOY_ASIX_XM;
					else if (event.jaxis.value > TRESHOLD) Back = JOY_ASIX_XP;
				}
				if (event.jaxis.axis==1) {  //Up-Down
					if (event.jaxis.value < -TRESHOLD)  Back = JOY_ASIX_YM;
					else if (event.jaxis.value > TRESHOLD)  Back = JOY_ASIX_YP;
				}
				break;
			}
			}
		}
	}
	return Back;
}

int get_keyboard(void) {
	SDL_Event event;
	int Back=-2;
	long timer;

	timer=time(NULL)+10;

	while(Back==-2) {

		if (time(NULL)>timer) {
			Back=-1;
		}

		if (SDL_PollEvent(&event)) {
			if (event.type==SDL_QUIT) {
				quit();
			}
			if (event.type==SDL_KEYDOWN) {
				Back=event.key.keysym.sym;
				if (Back == SDLK_ESCAPE) Back=-1;
				//Reserved key
				if ((Back == SDLK_F10) || (Back == SDLK_F5) ||
						(Back == SDLK_F7) || (Back == SDLK_BACKQUOTE)) {
					Back=-2;
				}
			}
		}
	}

	return Back;
}

int button_map[] = {
		BUTTON_UP,
		BUTTON_DOWN,
		BUTTON_LEFT,
		BUTTON_RIGHT,
		BUTTON_A,
		BUTTON_B,
		BUTTON_L,
		BUTTON_R,
		BUTTON_SELECT,
		BUTTON_START
};

int key_map(SDLKey key_sym) {
	int i;

	for(i=0;i<PLAT_KEY_COUNT;i++) {
		if (keyboard_config_map[i]==key_sym) return button_map[i];
	}
	return BUTTON_NONE;
}

int joy_map(u32 button) {
	int i;

	for(i=0;i<PLAT_BUTTON_COUNT;i++) {
		if (gamepad_config_map[i]==button) return button_map[i];
	}
	return BUTTON_NONE;
}
