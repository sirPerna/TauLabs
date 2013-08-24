/**
 ******************************************************************************
 * @file       leapcontrolgadgetwidget.h
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

#ifndef LEAPCONTROLGADGETWIDGET_H_
#define LEAPCONTROLGADGETWIDGET_H_

#include <QtGui/QLabel>

class Ui_LeapControl;

class LeapControlGadgetWidget : public QLabel
{
    Q_OBJECT

public:
    LeapControlGadgetWidget(QWidget *parent = 0);
    ~LeapControlGadgetWidget();

signals:

public slots:

protected slots:

private:
    Ui_LeapControl *m_leapcontrol;
};

#endif /* LEAPCONTROLGADGETWIDGET_H_ */
