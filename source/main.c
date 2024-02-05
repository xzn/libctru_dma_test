/*
	Hello World example made by Aurelio Mannara for libctru
	This code was modified for the last time on: 12/12/2014 21:00 UTC+1
*/

#include <3ds.h>
#include <stdio.h>
#include <memory.h>
#include <inttypes.h>

static void pause(u32 key) {
	// Main loop
	while (aptMainLoop())
	{
		svcSleepThread(1000000);

		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown() | hidKeysDownRepeat();

		if (key) {
			if (kDown & key) break; // break in order to return to hbmenu
		} else {
			if (kDown != 0) break;
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
}

#define BUF_COUNT (0x10000)
static u32 src[BUF_COUNT];
static u32 dst[BUF_COUNT];

static void clearDst(void) {
	memset(dst, 0, sizeof(dst));
	svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)dst, sizeof(dst));
}

static void testAndPrintResult(void) {
	for (u32 i = 0; i < BUF_COUNT; ++i) {
		src[i] = i;
	}
	svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)src, sizeof(src));
	clearDst();

	printf("src = { 0x%"PRIu32", 0x%"PRIu32", 0x%"PRIu32", 0x%"PRIu32", ... }\n", src[0], src[1], src[2], src[3]);

#define PITCH_FACTOR (2)
	u32 burstSize = 0x10;
	u32 transferSize = 0x100;
	u32 pitch = transferSize * PITCH_FACTOR;
	DmaConfig cfg = {
		.channelId = -1,
		.flags = DMACFG_WAIT_AVAILABLE | DMACFG_USE_SRC_CONFIG | DMACFG_USE_DST_CONFIG,
		.srcCfg = {
			.deviceId = -1,
			.allowedAlignments = 15,
			.burstSize = burstSize,
			.burstStride = burstSize,
			.transferSize = transferSize,
			.transferStride = pitch,
		},
		.dstCfg = {
			.deviceId = -1,
			.allowedAlignments = 15,
			.burstSize = burstSize,
			.burstStride = burstSize,
			.transferSize = transferSize,
			.transferStride = transferSize,
		},
	};

	printf(
		"dma from src to dst with\n"
		"transferSize = 0x%"PRIx32"\n"
		"srcCfg.transferStride = 0x%"PRIx32"\n"
		"dstCfg.transferStride = 0x%"PRIx32"\n", (u32)cfg.srcCfg.transferSize, (u32)cfg.srcCfg.transferStride, (u32)cfg.dstCfg.transferStride);
	pause(0);

	Handle hdma;
	svcStartInterProcessDma(&hdma, CUR_PROCESS_HANDLE, (u32)dst, CUR_PROCESS_HANDLE, (u32)src, BUF_COUNT / PITCH_FACTOR, &cfg);
	svcWaitSynchronization(hdma, -1);
	svcCloseHandle(hdma);

	printf("checking src at src[i * srcCfg.transferStride]\n");
	printf("     for dst at dst[i * dstCfg.transferStride]\n");
	printf("they should be equal\n");
	pause(0);

	for (u32 i = 1; i <= 0x10; ++i) {
		u32 srcIdx = i * cfg.srcCfg.transferStride;
		printf("src[0x%"PRIx32"] = 0x%"PRIx32" expected\n",
			srcIdx,
			src[srcIdx]
		);
		u32 dstIdx = i * cfg.dstCfg.transferStride;
		printf("dst[0x%"PRIx32"] = 0x%"PRIx32" got\n",
			dstIdx,
			dst[dstIdx]
		);
		pause(0);
	}

	// switched case

	clearDst();
	printf("switching dstCfg. and srcCfg.transferStride\n");
	cfg.srcCfg.transferStride = transferSize;
	cfg.dstCfg.transferStride = pitch;
	pause(0);

	printf(
		"dma from src to dst with\n"
		"transferSize = 0x%"PRIx32"\n"
		"srcCfg.transferStride = 0x%"PRIx32"\n"
		"dstCfg.transferStride = 0x%"PRIx32"\n", (u32)cfg.srcCfg.transferSize, (u32)cfg.srcCfg.transferStride, (u32)cfg.dstCfg.transferStride);
	pause(0);

	svcStartInterProcessDma(&hdma, CUR_PROCESS_HANDLE, (u32)dst, CUR_PROCESS_HANDLE, (u32)src, BUF_COUNT / PITCH_FACTOR, &cfg);
	svcWaitSynchronization(hdma, -1);
	svcCloseHandle(hdma);

	printf("checking src at src[i * dstCfg.transferStride]\n");
	printf("     for dst at dst[i * srcCfg.transferStride]\n");
	printf("they should be NOT equal\n");
	pause(0);

	for (u32 i = 1; i <= 0x10; ++i) {
		u32 srcIdx = i * cfg.dstCfg.transferStride;
		printf("src[0x%"PRIx32"] = 0x%"PRIx32" expected\n",
			srcIdx,
			src[srcIdx]
		);
		u32 dstIdx = i * cfg.srcCfg.transferStride;
		printf("dst[0x%"PRIx32"] = 0x%"PRIx32" got\n",
			dstIdx,
			dst[dstIdx]
		);
		pause(0);
	}
}

int main(int argc, char **argv)
{
	gfxInitDefault();

	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_TOP, NULL);

	testAndPrintResult();

	printf("Press Start to exit.");

	pause(KEY_START);

	gfxExit();
	return 0;
}
