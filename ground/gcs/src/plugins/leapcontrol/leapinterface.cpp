/**
 ******************************************************************************
 * @file       leapinterface.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      Interface to control the leap controller
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup LeapControl Leap Controller plugin
 * @{
 * @brief A gadget to control the UAV from a Leap Controller
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

#include <QDebug>
#include "leapinterface.h"

LeapInterface::LeapInterface(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Creating listener";
    m_timer = new QTimer(this);
}

//! Start receiveing events
void LeapInterface::getUpdate()
{
    Frame frame = m_controller.frame();

    if (hand_present && frame.hands().isEmpty()) {
        // hand disappeared
        emit handLost();
        hand_present = false;
    }

    if (!hand_present && !frame.hands().isEmpty()) {
        // hand found
        emit handFound();
        hand_present = true;
    }

    if (!frame.hands().isEmpty()) {
        // Get the position from the first hand
        const Hand hand = frame.hands()[0];
        const Vector normal = hand.palmNormal();
        const Vector direction = hand.direction();

        roll = -normal.roll() * RAD_TO_DEG;
        pitch = direction.pitch() * RAD_TO_DEG;
        yaw = direction.yaw() * RAD_TO_DEG;

        // Convert from Leap format to NED frame (in mm)
        x_pos = -hand.palmPosition()[2];
        y_pos = hand.palmPosition()[0];
        z_pos = -hand.palmPosition()[1];
    }

    emit handUpdated(hand_present, x_pos, y_pos, z_pos, roll, pitch, yaw);
}

void LeapInterface::start()
{
    m_timer->setInterval(100);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(getUpdate()));
    m_timer->start();
}

//! Stop receiveing events
void LeapInterface::stop()
{
    m_timer->stop();
    disconnect(this, SLOT(getUpdate()));
}
