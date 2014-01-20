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

#include "pios_ads1299.h"

#include "eegdata.h"
#include "eegsettings.h"
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
static void settingsUpdatedCb(UAVObjEvent * objEv);

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
#if defined(SIMULATE)
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
#else

	EEGSettingsConnectCallback(&settingsUpdatedCb);
	settingsUpdatedCb(NULL);

	struct pios_ads1299_data data;
	uint32_t sample = 0;
	while(1) {
		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		xQueueHandle queue;
		queue = PIOS_SENSORS_GetQueue(PIOS_SENSOR_EEG);
		if(queue == NULL || xQueueReceive(queue, (void *) &data, MS2TICKS(50)) == errQUEUE_EMPTY) {
			continue;
		}

		EEGDataData eegData;
		for (uint32_t i = 0; i < 8; i ++) {
			eegData.Data[i] = data.channels[i];
		}
		eegData.Sample = sample++;
		EEGDataSet(&eegData);

	}
#endif
}

static void settingsUpdatedCb(UAVObjEvent * ev) 
{
	EEGSettingsData eegSettings;
	EEGSettingsGet(&eegSettings);

	switch(eegSettings.SamplingRate) {
	case EEGSETTINGS_SAMPLINGRATE_250:
		PIOS_ADS1299_SetSamplingRate(ADS1299_250_SPS);
		break;
	case EEGSETTINGS_SAMPLINGRATE_500:
		PIOS_ADS1299_SetSamplingRate(ADS1299_500_SPS);
		break;
	case EEGSETTINGS_SAMPLINGRATE_1000:
		PIOS_ADS1299_SetSamplingRate(ADS1299_1000_SPS);
		break;
	case EEGSETTINGS_SAMPLINGRATE_2000:
		PIOS_ADS1299_SetSamplingRate(ADS1299_2000_SPS);
		break;
	}

	PIOS_ADS1299_EnableImpedance(eegSettings.ImpedanceMonitoring == EEGSETTINGS_IMPEDANCEMONITORING_TRUE);
}
