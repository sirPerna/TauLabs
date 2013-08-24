/**
 ******************************************************************************
 * @file       leapcontrolgadget.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
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
#include "leapcontrolgadget.h"
#include "leapcontrolgadgetwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include <QDebug>

LeapControlGadget::LeapControlGadget(QString classId, LeapControlGadgetWidget *widget, QWidget *parent, QObject *plugin) :
        IUAVGadget(classId, parent),
        m_widget(widget)
{
    Q_UNUSED(plugin)
}

LeapControlGadget::~LeapControlGadget()
{
    delete m_widget;
}

void LeapControlGadget::loadConfiguration(IUAVGadgetConfiguration* config)
{
    Q_UNUSED(config);
}

/**
 * @}
 * @}
 */
