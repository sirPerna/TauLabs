/**
 ******************************************************************************
 * @file       PFD.java
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      The PFD display fragment
 * @see        The GNU Public License (GPL) Version 3
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

package org.taulabs.androidgcs.fragments;

import org.taulabs.androidgcs.R;
import org.taulabs.uavtalk.UAVObject;
import org.taulabs.uavtalk.UAVObjectManager;


import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class Graph extends ObjectManagerFragment {

	private static final String TAG = Graph.class.getSimpleName();
	private static final int LOGLEVEL = 0;
	// private static boolean WARN = LOGLEVEL > 1;
	private static final boolean DEBUG = LOGLEVEL > 0;

	// @Override
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		return inflater.inflate(R.layout.graph, container, false);
	}

	@Override
	public void onConnected(UAVObjectManager objMngr) {
		super.onConnected(objMngr);
		if (DEBUG)
			Log.d(TAG, "On connected");

		UAVObject obj = objMngr.getObject("EEGData");
		if (obj != null) {
			registerObjectUpdates(obj);
			objectUpdated(obj);
		}


	}
	
	private int missedUpdates;
	private int lastCounter;

	/**
	 * Called whenever any objects subscribed to via registerObjects
	 */
	@Override
	public void objectUpdated(UAVObject obj) {
		if (DEBUG)
			Log.d(TAG, "Updated");

		if (obj == null)
			return;

		if (obj.getName().compareTo("EEGData") == 0) {
			int counter = (int) obj.getField("Sample").getDouble();
			
			if ((counter - lastCounter) > 1)
				missedUpdates++;
			
			lastCounter = counter;
			
			TextView missedUpdatesView = (TextView) getActivity().findViewById(R.id.missedUpdates);
			missedUpdatesView.setText(String.valueOf(missedUpdates));
		}

	}

	@Override
	protected String getDebugTag() {
		return TAG;
	}

}
