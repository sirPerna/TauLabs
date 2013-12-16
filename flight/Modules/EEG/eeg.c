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
#include "sin_lookup.h"

#include "eegdata.h"
#include "eegstatus.h"

// Private constants
#define STACK_SIZE 1024
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
	xTaskCreate(EegTask, (signed char *)"EEG", STACK_SIZE/4, NULL, TASK_PRIORITY, &taskHandle);

	// Pretending it is the actuator task for now
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
	const float FREQUENCY = 2;
	const float SAMPLE_DT = UPDATE_PERIOD / 1000.0f;

	sin_lookup_initialize();

	EEGDataData data;
	data.Sample = 0;

	// Main task loop
	while (1) {
		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		data.Sample++;
		float t = data.Sample * SAMPLE_DT;
		float analog = PIOS_ADC_GetChannelVolt(0) / 10.0f;

		for (uint32_t i = 0; i < EEGDATA_DATA_NUMELEM; i++)
			data.Data[i] = sin_lookup_rad(t * 2 * PI * FREQUENCY + PI * i / 4) + analog;

		EEGDataSet(&data);

		vTaskDelay(MS2TICKS(UPDATE_PERIOD));
	}
}
