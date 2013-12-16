###############################################################################
# @file       Makefile
# @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
# @addtogroup 
# @{
# @addtogroup 
# @{
# @brief Makefile to build UAVObject code for Sparky board.
###############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

# These are the UAVObjects supposed to be build as part of the Tau Labs target
# (all architectures)

UAVOBJSRCFILENAMES =
UAVOBJSRCFILENAMES += accels
UAVOBJSRCFILENAMES += accessorydesired
UAVOBJSRCFILENAMES += actuatorcommand
UAVOBJSRCFILENAMES += actuatordesired
UAVOBJSRCFILENAMES += actuatorsettings
UAVOBJSRCFILENAMES += adcrouting
UAVOBJSRCFILENAMES += attitudeactual
UAVOBJSRCFILENAMES += attitudesettings
UAVOBJSRCFILENAMES += baroaltitude
UAVOBJSRCFILENAMES += eegdata
UAVOBJSRCFILENAMES += eegstatus
UAVOBJSRCFILENAMES += firmwareiapobj
UAVOBJSRCFILENAMES += flightbatterysettings
UAVOBJSRCFILENAMES += flightbatterystate
UAVOBJSRCFILENAMES += flighttelemetrystats
UAVOBJSRCFILENAMES += flightstatus
UAVOBJSRCFILENAMES += gcsreceiver
UAVOBJSRCFILENAMES += gcstelemetrystats
UAVOBJSRCFILENAMES += gpsposition
UAVOBJSRCFILENAMES += gpsvelocity
UAVOBJSRCFILENAMES += gyros
UAVOBJSRCFILENAMES += gyrosbias
UAVOBJSRCFILENAMES += homelocation
UAVOBJSRCFILENAMES += hwsparky
UAVOBJSRCFILENAMES += insstate
UAVOBJSRCFILENAMES += inssettings
UAVOBJSRCFILENAMES += magnetometer
UAVOBJSRCFILENAMES += magbias
UAVOBJSRCFILENAMES += manualcontrolcommand
UAVOBJSRCFILENAMES += manualcontrolsettings
UAVOBJSRCFILENAMES += mixersettings
UAVOBJSRCFILENAMES += mixerstatus
UAVOBJSRCFILENAMES += modulesettings
UAVOBJSRCFILENAMES += nedaccel
UAVOBJSRCFILENAMES += nedposition
UAVOBJSRCFILENAMES += objectpersistence
UAVOBJSRCFILENAMES += poilocation
UAVOBJSRCFILENAMES += positionactual
UAVOBJSRCFILENAMES += ratedesired
UAVOBJSRCFILENAMES += receiveractivity
UAVOBJSRCFILENAMES += sensorsettings
UAVOBJSRCFILENAMES += stabilizationdesired
UAVOBJSRCFILENAMES += stabilizationsettings
UAVOBJSRCFILENAMES += stateestimation
UAVOBJSRCFILENAMES += systemalarms
UAVOBJSRCFILENAMES += systemsettings
UAVOBJSRCFILENAMES += systemstats
UAVOBJSRCFILENAMES += taskinfo
UAVOBJSRCFILENAMES += trimangles
UAVOBJSRCFILENAMES += trimanglessettings
UAVOBJSRCFILENAMES += velocityactual
UAVOBJSRCFILENAMES += vibrationanalysissettings
UAVOBJSRCFILENAMES += vibrationanalysisoutput
UAVOBJSRCFILENAMES += watchdogstatus

UAVOBJSRC = $(foreach UAVOBJSRCFILE,$(UAVOBJSRCFILENAMES),$(OPUAVSYNTHDIR)/$(UAVOBJSRCFILE).c )
UAVOBJDEFINE = $(foreach UAVOBJSRCFILE,$(UAVOBJSRCFILENAMES),-DUAVOBJ_INIT_$(UAVOBJSRCFILE) )
