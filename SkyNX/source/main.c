// The following ffmpeg code is inspired by an official ffmpeg example, so here is its Copyright notice:

/*
 * Copyright (c) 2012 Stefano Sabatini
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.


*
 * @file
 * Demuxing and decoding example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c

*/

#include <libavutil/samplefmt.h>

#include <switch.h>
#include <SDL.h>

#include "context.h"
#include "input.h"
#include "video.h"
#include "network.h"
#include "renderer.h"
#include "audio.h"
static const SocketInitConfig socketInitConf = {
    .bsdsockets_version = 1,

    .tcp_tx_buf_size = 0x80000,
    .tcp_rx_buf_size = 0x100000,
    .tcp_tx_buf_max_size = 0x400000,
    .tcp_rx_buf_max_size = 0x400000,

    .udp_tx_buf_size = 0x1400,
    .udp_rx_buf_size = 0x3500,

    .sb_efficiency = 2,
};
u32 gyroHandles[4];
void initGyro()
{
    hidGetSixAxisSensorHandles(&gyroHandles[0], 2, CONTROLLER_PLAYER_1, TYPE_JOYCON_PAIR);
    hidGetSixAxisSensorHandles(&gyroHandles[2], 1, CONTROLLER_PLAYER_1, TYPE_PROCONTROLLER);
    hidGetSixAxisSensorHandles(&gyroHandles[3], 1, CONTROLLER_HANDHELD, TYPE_HANDHELD);
    hidStartSixAxisSensor(gyroHandles[0]);
    hidStartSixAxisSensor(gyroHandles[1]);
    hidStartSixAxisSensor(gyroHandles[2]);
    hidStartSixAxisSensor(gyroHandles[3]);
}
void unInitGyro()
{

    hidStopSixAxisSensor(gyroHandles[0]);
    hidStopSixAxisSensor(gyroHandles[1]);
    hidStopSixAxisSensor(gyroHandles[2]);
    hidStopSixAxisSensor(gyroHandles[3]);
}
void switchInit()
{
    plInitialize();
    pcvInitialize();
    romfsInit();
    networkInit(&socketInitConf);
    audoutInitialize();
    audoutStartAudioOut();
}

void switchDestroy()
{
    audoutStopAudioOut();
    audoutExit();
    networkDestroy();
    pcvExit();
    plExit();
}

void startInput()
{
    static Thread inputHandlerThread;
    threadCreate(&inputHandlerThread, inputHandlerLoop, NULL, NULL, 0x1000, 0x2b, 0);
    threadStart(&inputHandlerThread);
}

void startAudio()
{
    static Thread audioHandlerThread;
    // On same thread as input and preemptive
    threadCreate(&audioHandlerThread, audioHandlerLoop, NULL, NULL, 0x10000, 0x20, 1);
    threadStart(&audioHandlerThread);
}

void startRender(VideoContext *videoContext)
{
    static Thread renderThread;
    threadCreate(&renderThread, videoLoop, videoContext, NULL, 0x800000, 0x2b, 2);
    threadStart(&renderThread);
}

RenderContext *renderContext = NULL;
VideoContext *videoContext = NULL;
void init()
{
    /* Init all switch required systems */
    switchInit();
    pcvSetClockRate(PcvModule_CpuBus, 1785000000); //Overclock CPU

    renderContext = createRenderer();
    videoContext = createVideoContext();
    videoContext->renderContext = renderContext;
    startAudio();
    startInput();
    startRender(videoContext);
    initGyro();
}
void unInit()
{
    freeRenderer(renderContext);
    freeVideoContext(videoContext);
    unInitGyro();
    pcvSetClockRate(PcvModule_CpuBus, 1020000000); //Reset CPU clock to default
}
int main(int argc, char **argv)
{
    init();

    while (appletMainLoop())
    {
        if (isVideoActive(renderContext))
        {
            displayFrame(renderContext);
        }
        else
        {
            drawSplash(renderContext);
        }
        svcSleepThread(14285714ULL); //Nano sleep to keep at 70fps to handle drop frames without stutter
    }
    /* Deinitialize all used systems */
    unInit();
}
