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

#include "uavtalk.h"
#include "uavtalk_priv.h"

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
static bool impedance_monitoring;

// Private functions
static void EegTask(void *parameters);
static void settingsUpdatedCb(UAVObjEvent * objEv);
static void sendEEGObject(EEGDataData *data);

int32_t EEGInitialize()
{
	EEGDataInitialize();
	EEGStatusInitialize();
	EEGSettingsInitialize();

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

	const uint32_t CHANNELS = 8;

	float cos_accum[CHANNELS], sin_accum[CHANNELS];
	const float cos_table[] = {1, 0, -1, 0};
	const float sin_table[] = {0, 1, 0, -1};

	while(1) {
		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		xQueueHandle queue;
		queue = PIOS_SENSORS_GetQueue(PIOS_SENSOR_EEG);
		if(queue == NULL || xQueueReceive(queue, (void *) &data, MS2TICKS(50)) == errQUEUE_EMPTY) {
			continue;
		}

		EEGDataData eegData;
		for (uint32_t i = 0; i < CHANNELS; i ++) {
			eegData.Data[i] = data.channels[i];
		}
		eegData.Sample = sample++;
		sendEEGObject(&eegData);

		// Measure the power at fDR / 4
		if (impedance_monitoring) {

			EEGStatusData eegStatus;

			// Sampling rate at 500 sps, impedance pulse at 1/4 that and
			// then impedance measured at 1/8 that (so updated at 15 Hz)
			// IMPEDANCE_ALPHA of 0.97 gives about one second time
			// constant
			const uint32_t PERIODS = 8;
			const float IMPEDANCE_ALPHA = 0.97f;

			for (uint32_t i = 0; i < CHANNELS; i++) {
				cos_accum[i] += eegData.Data[i] * cos_table[sample % 4];
				sin_accum[i] += eegData.Data[i] * sin_table[sample % 4];
			}

			if (sample % (4 * PERIODS) == 0) {
				
				for (uint32_t i = 0; i < CHANNELS; i++) {

					float a = cos_accum[i] / 2.0f / PERIODS;
					float b = sin_accum[i] / 2.0f / PERIODS;
					float amp = sqrtf(a*a + b*b); //uV

					// Fudge factor. This is a square wave pulse and we are measuring
					// the amplitude of the base component. This compensates.
					amp *= 0.7071f; // sqrt(2)

					// +/- 6nA square wave pulse. Convert voltage to nV
					float impedance =  amp * 1000.0f / 6.0f;

					eegStatus.Impedance[i] = eegStatus.Impedance[i] * IMPEDANCE_ALPHA +
					                         (1 - IMPEDANCE_ALPHA) * impedance;

					// Reset the accumulator
					cos_accum[i] = 0;
					sin_accum[i] = 0;
				}
			}

			EEGStatusSet(&eegStatus);
		}

	}
#endif
}

/**
 * This replicates the UAVTalk method because we want to
 * do this as efficiently as possible and without the event
 * system being hammered
 */
static void sendEEGObject(EEGDataData *data)
{
	int32_t length;
	int32_t dataOffset;
	uint32_t objId;
	uint8_t txBuffer[256];

	// Setup type and object id fields
	objId = EEGDATA_OBJID;
	txBuffer[0] = UAVTALK_SYNC_VAL;  // sync byte
	txBuffer[1] = UAVTALK_TYPE_OBJ;
	// data length inserted here below
	txBuffer[4] = (uint8_t)(objId & 0xFF);
	txBuffer[5] = (uint8_t)((objId >> 8) & 0xFF);
	txBuffer[6] = (uint8_t)((objId >> 16) & 0xFF);
	txBuffer[7] = (uint8_t)((objId >> 24) & 0xFF);
	
	// Setup instance ID if one is required
	dataOffset = 8;

	length = EEGDATA_NUMBYTES;
	memcpy(&txBuffer[dataOffset], data, length);
	
	// Store the packet length
	txBuffer[2] = (uint8_t)((dataOffset+length) & 0xFF);
	txBuffer[3] = (uint8_t)(((dataOffset+length) >> 8) & 0xFF);
	
	// Calculate checksum
	txBuffer[dataOffset+length] = PIOS_CRC_updateCRC(0, txBuffer, dataOffset+length);
	uint16_t tx_msg_len = dataOffset+length+UAVTALK_CHECKSUM_LENGTH;

	PIOS_COM_SendBufferNonBlocking(getComPort(), txBuffer, tx_msg_len);
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

	impedance_monitoring = eegSettings.ImpedanceMonitoring == EEGSETTINGS_IMPEDANCEMONITORING_TRUE;
	PIOS_ADS1299_EnableImpedance(impedance_monitoring);
}
