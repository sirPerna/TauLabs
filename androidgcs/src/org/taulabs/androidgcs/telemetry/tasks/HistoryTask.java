/**
 ******************************************************************************
 * @file       HistoryTask.java
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      An @ref ITelemTask that maintains a few second history
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
package org.taulabs.androidgcs.telemetry.tasks;

import java.util.Observable;
import java.util.Observer;

import org.taulabs.uavtalk.UAVObject;
import org.taulabs.uavtalk.UAVObjectManager;

import android.content.Context;
import android.util.Log;

public class HistoryTask implements ITelemTask {

	final String TAG = HistoryTask.class.getSimpleName();
	final boolean VERBOSE = false;
	final boolean DEBUG = false;

	private UAVObjectManager objMngr;
	private UAVObject eegDataObject; 
	
	public final static int HISTORY_LEN = 1000;
	public final static int CHANNELS    = 8;

	private float history[][] = new float[CHANNELS][HISTORY_LEN];
	float tempValues[] = new float[HISTORY_LEN];

	@Override
	public void connect(UAVObjectManager o, Context context) {
		objMngr = o;

		// When new objects are registered, ensure we listen
		// to them
		o.addNewObjectObserver(newObjObserver);
		o.addNewInstanceObserver(newObjObserver);

		UAVObject obj = o.getObject("EEGData");
		if (obj != null)
			registerEeg(obj);
	}

	@Override
	public void disconnect() {
		if (objMngr != null && eegDataObject != null) {
			eegDataObject.removeUpdatedObserver(objUpdatedObserver);
		}
	}
	
	private void registerEeg(UAVObject obj) {
		Log.d(TAG, "Registered EEG");
		eegDataObject = obj;
		obj.addUpdatedObserver(objUpdatedObserver);
	}
	
	//! Observer to catch when new objects or instances are registered
	private final Observer newObjObserver = new Observer() {
		@Override
		public void update(Observable observable, Object data) {
			UAVObject obj = (UAVObject) data;
			if (obj.getName().compareTo("EEGData") == 0) {
				registerEeg(obj);
			}
		}
	};
	
	private int totalSamples;
	private int lastCounter;
	private int missedUpdates;

	//! Observer to catch when objects are updated
	private final Observer objUpdatedObserver = new Observer() {
		@Override
		public void update(Observable observable, Object data) {
			UAVObject obj = (UAVObject) data;
			if (obj.getName().compareTo("EEGData") == 0) {
				
				totalSamples++;
				
				// Monitor for any missed updates
				int counter = (int) obj.getField("Sample").getDouble();
				if ((counter - lastCounter) > 1) {
					missedUpdates++;
				}
				lastCounter = counter;
				
				if (counter % 1000 == 0)
					Log.d(TAG, "Missed updates: " + missedUpdates);
				
				for (int i = 0; i < CHANNELS; i++) {
					// Get the new data point
					float data_point = (float) obj.getField("Data").getDouble(i);
					
					// Copy the data shifted by one position into a temporary array
					System.arraycopy(history[i], 1, tempValues, 0, HISTORY_LEN-1);
					tempValues[HISTORY_LEN-1] = data_point;
					System.arraycopy(tempValues, 0, history[i], 0, HISTORY_LEN);
				}
			}
		}
	};

	public class Stats {
		public int skippedCounters;
		public int lastCounter;
	};

	//! Return an object with the logging stats
	public Stats getStats() {
		Stats s = new Stats();
		return s;
	}

	//! Accessor to get the sampling rate of the data
	public float getT0() {
		return totalSamples / getSamplingRate();
	}
	
	//! Accessor to get the sampling rate of the data
	public float getSamplingRate() {
		return 200;
	}
	
	//! Accessor to get the data history
	public float [][] getHistory() {
		return history;
	}

}
