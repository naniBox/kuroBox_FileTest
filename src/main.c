/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include "ff.h"

//-----------------------------------------------------------------------------
#define DEBG ((BaseSequentialStream *)&SD2)

//-----------------------------------------------------------------------------
#define kbg_setGPIO(on,port,pin) 	palWritePad(port,pin,on)
#define kbg_getGPIO(port,pin)		palReadPad(port,pin)

//-----------------------------------------------------------------------------
#define kbg_setLED1(on)				kbg_setGPIO(on, GPIOB, GPIOB_LED1)
#define kbg_getLED1()				kbg_getGPIO(GPIOB, GPIOB_LED1)

#define kbg_setLED2(on)				kbg_setGPIO(on, GPIOB, GPIOB_LED2)
#define kbg_getLED2()				kbg_getGPIO(GPIOB, GPIOB_LED2)

#define kbg_setLED3(on)				kbg_setGPIO(on, GPIOA, GPIOA_LED3)
#define kbg_getLED3()				kbg_getGPIO(GPIOA, GPIOA_LED3)

//-----------------------------------------------------------------------------
static const SDCConfig sdio_cfg = {
	0
};

//-----------------------------------------------------------------------------
void
panic_it(int times)
{
	while(1)
	{
		chprintf(DEBG, "Error: %d\n\r", times);
		for (int i = 0 ; i < (times+1) ; i++ )
		{
			kbg_setLED3(1);
			chThdSleepMilliseconds(250);
			kbg_setLED3(0);
			chThdSleepMilliseconds(250);
		}
		chThdSleepMilliseconds(2000);
	}

}

//-----------------------------------------------------------------------------
uint8_t buf[16*1024];
FATFS SDC_FS;
FIL kbfile;
FRESULT stat;

//-----------------------------------------------------------------------------
int
main(void)
{
	halInit();
	chSysInit();
	sdcStart(&SDCD1, &sdio_cfg);
	sdStart(&SD2, NULL);

	chprintf(DEBG, "%s\n\r\n\r", BOARD_NAME);

	chThdSleepMilliseconds(200);
	if (sdcConnect(&SDCD1) != CH_SUCCESS)
	{
		panic_it(1);
	}

	chThdSleepMilliseconds(200);
	stat = f_mount(0, &SDC_FS);
	if (stat != FR_OK)
	{
		panic_it(2);
	}

	chThdSleepMilliseconds(200);
	if ( sdc_lld_is_write_protected(&SDCD1) )
	{
		panic_it(3);
	}

	chThdSleepMilliseconds(200);
	stat = f_open(&kbfile, "KB_FS.TST", FA_WRITE|FA_CREATE_ALWAYS);
	if (stat != FR_OK)
	{
		panic_it(4);
	}

	for (uint32_t i = 0 ; i < sizeof(buf) ; i++ )
		buf[i] = i%256;

	#define RUNS 100
	while( 1 )
	{
		uint32_t times[RUNS];
		memset(times, 0, sizeof(times));

		for ( uint32_t run = 0 ; run < RUNS ; run++ )
		{
			kbg_setLED1(0);
			uint32_t t0 = halGetCounterValue();
			UINT bytes_written = 0;
			stat = f_write(&kbfile, buf, sizeof(buf), &bytes_written);
			uint32_t t1 = halGetCounterValue();
			times[run] += t1 - t0;
			kbg_setLED1(1);
			//chThdSleepMilliseconds(100);
		}
		f_sync(&kbfile);

		uint32_t max = 0;
		uint32_t min = 0xffffffff;
		uint32_t total_times = 0;
		for ( uint32_t run = 0 ; run < RUNS ; run++ )
		{
			if ( min > times[run] ) min = times[run];
			if ( max < times[run] ) max = times[run];
			total_times += times[run];

		}
		uint32_t avg = total_times / RUNS;

		float fmax = max / 168000.0f;
		float fmin = min / 168000.0f;
		float favg = avg / 168000.0f;
		float ftot = total_times / 168000.0f;

		static int loop = 0;

		chprintf(DEBG, "Times: %4d; ", loop++);
		chprintf(DEBG, " min: %9fms;", fmin);
		chprintf(DEBG, " max: %9fms;", fmax);
		chprintf(DEBG, " avg: %9fms;", favg);
		chprintf(DEBG, " total: %9fms;", ftot);
		chprintf(DEBG, " speed: %9fMB/s\n\r", (sizeof(buf)*100.0f/ftot)/1000.0f);
	}
}
