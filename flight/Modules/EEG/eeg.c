/**
 ******************************************************************************
 *
 * @file       eeg.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      Reads EEG data and packages it into a UAVO
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "openpilot.h"

#include "physical_constants.h"

#include "eegdata.h"
#include "eegstatus.h"

// Private constants
#define STACK_SIZE configMINIMAL_STACK_SIZE
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

// Private types
#define UPDATE_PERIOD 5 /* 200 Hz */

// Private variables
static xTaskHandle taskHandle;

// Private functions
static void EegTask(void *parameters);

int32_t EEGInitialize()
{
	EEGDataInitialize();
	EEGStatusInitialize();

	return 0;
}

int32_t EEGStart()
{
	// Start main task
	xTaskCreate(EegTask, (signed char *)"EEG", STACK_SIZE, NULL, TASK_PRIORITY, &taskHandle);

	TaskMonitorAdd(TASKINFO_RUNNING_ACTUATOR, taskHandle);
	PIOS_WDG_RegisterFlag(PIOS_WDG_ACTUATOR);

	return 0;
}

MODULE_INITCALL(EEGInitialize, EEGStart);

/**
 * Module thread, should not return.
 */
static void EegTask(void *parameters)
{
	const float FREQUENCY = 10;

	portTickType lastSysTime;

	EEGDataData data;
	data.Sample = 0;

	// Main task loop
	lastSysTime = xTaskGetTickCount();
	while (1) {
		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		data.Sample++;
		data.Data[0] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 0 / 4);
		data.Data[1] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 1 / 4);
		data.Data[2] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 2 / 4);
		data.Data[3] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 3 / 4);
		data.Data[4] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 4 / 4);
		data.Data[5] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 5 / 4);
		data.Data[6] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 6 / 4);
		data.Data[7] = sinf(data.Sample / 200.f * 2 * PI * FREQUENCY + PI * 7 / 4);

		EEGDataSet(&data);

		vTaskDelayUntil(&lastSysTime, MS2TICKS(UPDATE_PERIOD));
	}
}
