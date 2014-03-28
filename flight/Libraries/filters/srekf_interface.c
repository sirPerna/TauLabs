/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @file       srekf_interface.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief      Interface from the SE(3)+ infrastructure to the SREKF algorithm
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

#include "filter_interface.h"
#include "filter_infrastructure_se3.h"

#include "attitude_ekf.h"
#include "quaternion.h"

static int32_t srekf_interface_init(uintptr_t *id);
static int32_t srekf_interface_reset(uintptr_t id);
static int32_t srekf_interface_update(uintptr_t id, float gyros[3], float accels[3], 
		float mag[3], float pos[3], float vel[3], float baro[1],
		float airspeed[1], float dt);
static int32_t srekf_interface_get_state(uintptr_t id, float pos[3], float vel[3],
		float attitude[4], float gyro_bias[3], float airspeed[1]);

const struct filter_driver srekf_filter_driver = {
	.class = FILTER_CLASS_S3,

	// this will initialize the SE(3)+ infrastrcture too
	.init = srekf_interface_init,

	// connects the SE(3)+ queues
	.start = filter_infrastructure_se3_start,
	.reset = srekf_interface_reset,
	.process = filter_infrastructure_se3_process,
	.sub_driver = {
		.driver_s3 = {
			.update_filter = srekf_interface_update,
			.get_state = srekf_interface_get_state,
			.magic = FILTER_S3_MAGIC,
		}
	}
};

enum srekf_interface_magic {
	SREKF_INTERFACE_MAGIC = 0xB34BFE8C,
};

struct srekf_interface_data {
	struct filter_infrastructure_se3_data *s3_data;
	Attitude_Estimation_States_Type state;
	Attitude_Estimation_Settings_Type settings;
	enum srekf_interface_magic magic;
};

static struct srekf_interface_data * srekf_interface_alloc()
{
	struct srekf_interface_data *srekf;

	srekf = pvPortMalloc(sizeof(*srekf));

	srekf->magic        = SREKF_INTERFACE_MAGIC;

	return srekf;
}

/**
 * Validate a CF filter handle
 * @return true if a valid interface
 */
static bool srekf_interface_validate(struct srekf_interface_data *dev)
{
	if (dev == NULL)
		return false;
	if (dev->magic != SREKF_INTERFACE_MAGIC)
		return false;
	return true;
}

/**
 * Initialize this SREKF filter and the SE(3)+ infrastructure
 * @param[out]  id   the handle for this filter instance
 * @return 0 if successful, -1 if not
 */
static int32_t srekf_interface_init(uintptr_t *id)
{
	// Allocate the data structure
	struct srekf_interface_data * srekf_interface_data = srekf_interface_alloc();
	if (srekf_interface_data == NULL)
		return -1;

	// Initialize the infrastructure
	if (filter_infrastructure_se3_init(&srekf_interface_data->s3_data) != 0)
		return -2;
	
	// Return the handle
	(*id) = (uintptr_t) srekf_interface_data;

	return 0;
}


/********* formatting sensor data to the core math code goes below here *********/

/**
 * Reset the filter state to default
 * @param[in]  id        the filter handle to reset
 */
static int32_t srekf_interface_reset(uintptr_t id)
{
	struct srekf_interface_data *srekf = (struct srekf_interface_data *) id;
	if (!srekf_interface_validate(srekf))
		return -1;

	quaternion_t q_init;
	vector3f_t wb_init = {0.0f, 0.0f, 0.0f};
	vector3f_t acc_init = {0.0f, 0.0f, -9.81f};
	vector3f_t mag_init = {250.0f, 0.0f, 450.0f};

	/* Generate the starting guess quaternion */
	GenerateStartingGuess(&acc_init, &mag_init, &q_init);

	/* Initialize the estimation */
	AttitudeEstimationInit(&srekf->state, &srekf->settings, &q_init, &wb_init, 0.005f);

	return 0;
}

/**
 * get_sensors Update the filter one time step
 * @param[in] id         the running filter handle
 * @param[in] gyros      new gyro data [deg/s] or NULL
 * @param[in] accels     new accel data [m/s^2] or NULL
 * @param[in] mag        new mag data [mGau] or NULL
 * @param[in] pos        new position measurement in NED [m] or NULL
 * @param[in] vel        new velocity meansurement in NED [m/s] or NULL
 * @param[in] baro       new baro data [m] or NULL
 * @param[in] airspeed   estimate of the airspeed
 * @param[in] dt         time step [s]
 * @returns 0 if sufficient data to run update
 */
static int32_t srekf_interface_update(uintptr_t id, float gyros[3], float accels[3], 
		float mag[3], float pos[3], float vel[3], float baro[1],
		float airspeed[1], float dt)
{
	struct srekf_interface_data *srekf = (struct srekf_interface_data *) id;
	if (!srekf_interface_validate(srekf))
		return -1;

	static float mag_s[3] = {250.0f, 0.0f, 400.0f};
	if (mag != NULL) {
		// Repeat most recent mag measurement until filter handles asynchronous data
		mag_s[0] = mag[0];
		mag_s[1] = mag[1];
		mag_s[2] = mag[2];
	}

	InnovateAttitudeEKF(&srekf->state,
	                    &srekf->settings,
	                    gyros,
	                    accels,
	                    mag_s,
	                    0.0f,
	                    0.0f,
	                    dt);
	return 0;
}

/**
 * get_state Retrieve the state from the S(3) filter
 * any param can be null indicating it is not being fetched
 * @param[in]  id        the running filter handle
 * @param[out] pos       the updated position in NED [m]
 * @param[out] vel       the updated velocity in NED [m/s]
 * @param[out] attitude  the updated attitude quaternion
 * @param[out] gyro_bias the update gyro bias [deg/s]
 * @param[out] airspeed  estimate of the airspeed
 */
static int32_t srekf_interface_get_state(uintptr_t id, float pos[3], float vel[3],
		float attitude[4], float gyro_bias[3], float airspeed[1])
{
	struct srekf_interface_data *srekf = (struct srekf_interface_data *) id;
	if (!srekf_interface_validate(srekf))
		return -1;

	if (attitude) {
		attitude[0] = srekf->state.q.q0;
		attitude[1] = srekf->state.q.q1;
		attitude[2] = srekf->state.q.q2;
		attitude[3] = srekf->state.q.q3;
	}
	return 0;
}

/**
 * @}
 */
