/**
 ******************************************************************************
 * @file       leapinterface.h
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

#ifndef LEAPINTERFACE_H
#define LEAPINTERFACE_H

#include <QObject>
#include <QTimer>

#include <iostream>
#include "Leap.h"

using namespace Leap;

class LeapInterface : public QObject
{
    Q_OBJECT

public:
    explicit LeapInterface(QObject *parent = 0);

    /* Methods to control plugin */
    void start();
    void stop();

signals:
    void handFound();
    void handLost();
    void handUpdated(bool present, double x, double y, double z, double roll, double pitch, double yaw);

private slots:
    void getUpdate();

private:
    Controller  m_controller;
    QTimer    * m_timer;

    double x_pos, y_pos, z_pos;
    double roll, pitch, yaw;
    bool hand_present;
};

#endif // LEAPINTERFACE_H
