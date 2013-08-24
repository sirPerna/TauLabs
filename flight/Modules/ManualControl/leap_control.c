/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup Control Control Module
 * @{
 *
 * @file       leap_control.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      Use Leap Control for control source
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
#include "control.h"
#include "leap_control.h"
#include "transmitter_control.h"
#include "physical_constants.h"

#include "leapcontrol.h"
#include "manualcontrolcommand.h"
#include "stabilizationdesired.h"

#if !defined(COPTERCONTROL)

//! Initialize the tablet controller
int32_t leap_control_initialize()
{
	LeapControlInitialize();
	return 0;
}

//! Process updates for the leap controller
int32_t leap_control_update()
{
	// TODO: Determine what to do when there are insufficient updates
	// from the leap controller.
	return 0;
}

/**
 * Select and use leap control
 * @param [in] reset_controller True if previously another controller was used
 */
int32_t leap_control_select(bool reset_controller)
{
	LeapControlData leapControl;
	LeapControlGet(&leapControl);

	StabilizationDesiredData stabilizationDesired;
	StabilizationDesiredGet(&stabilizationDesired);
	if (leapControl.Present == LEAPCONTROL_PRESENT_TRUE) {
		stabilizationDesired.Roll      = leapControl.Roll;
		stabilizationDesired.Pitch     = leapControl.Pitch;
		stabilizationDesired.Yaw       = leapControl.Yaw / 5;
		ManualControlCommandThrottleGet(&stabilizationDesired.Throttle);
	} else {
		stabilizationDesired.Roll      = 0;
		stabilizationDesired.Pitch     = 0;
		stabilizationDesired.Yaw       = 0;
		stabilizationDesired.Throttle  = 0;
	}
	stabilizationDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabilizationDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabilizationDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
	StabilizationDesiredSet(&stabilizationDesired);

	return 0;
}

//! Get any control events
enum control_events leap_control_get_events()
{
	// For now ARM / DISARM events still come from the transmitter
	return transmitter_control_get_events();
}

#else

int32_t leap_control_initialize()
{
	return 0;
}

//! Process updates for the tablet controller
int32_t leap_control_update()
{
	return 0;
}

int32_t leap_control_select(bool reset_controller)
{
	return 0;
}

//! When not supported force disarming
enum control_events leap_control_get_events()
{
	return CONTROL_EVENTS_DISARM;
}

#endif

/**
 * @}
 * @}
 */
