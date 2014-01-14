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
 * Determine input/output com port as highest priority available 
 */
static uintptr_t getComPort() {
#if defined(PIOS_INCLUDE_USB)
	if ( PIOS_COM_Available(PIOS_COM_TELEM_USB) )
		return PIOS_COM_TELEM_USB;
	else
#endif /* PIOS_INCLUDE_USB */
		if ( PIOS_COM_Available(PIOS_COM_TELEM_RF) )
			return PIOS_COM_TELEM_RF;
		else
			return 0;
}

/**
 * Transmit data buffer to the modem or USB port.
 * \param[in] data Data buffer to send
 * \param[in] length Length of buffer
 * \return -1 on failure
 * \return number of bytes transmitted on success
 */
static int32_t pack_data(uint8_t * data, int32_t length)
{
	if( PIOS_COM_SendBufferNonBlocking(getComPort(), data, length) < 0)
		return -1;
	return length;
}

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
	UAVTalkConnection uavTalkCon;
	uavTalkCon = UAVTalkInitialize(&pack_data);

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

		UAVTalkSendObjectTimestamped(uavTalkCon, EEGDataHandle(), 0, false, 0);

	}
#endif
}
