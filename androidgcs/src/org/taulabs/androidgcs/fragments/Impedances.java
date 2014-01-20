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

import java.util.ArrayList;

import org.taulabs.androidgcs.ObjectManagerActivity;
import org.taulabs.androidgcs.R;
import org.taulabs.androidgcs.telemetry.tasks.HistoryTask;
import org.taulabs.uavtalk.UAVObject;
import org.taulabs.uavtalk.UAVObjectManager;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GraphViewDataInterface;
import com.jjoe64.graphview.GraphViewSeries;
import com.jjoe64.graphview.LineGraphView;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.Space;
import android.widget.TextView;
import android.widget.ToggleButton;

public class Impedances extends ObjectManagerFragment {

	private static final String TAG = Impedances.class.getSimpleName();
	private static final int LOGLEVEL = 0;
	// private static boolean WARN = LOGLEVEL > 1;
	private static final boolean DEBUG = LOGLEVEL > 0;

	private HistoryTask history;
	private ArrayList<TextView> impedanceList = new ArrayList<TextView>();
	private ArrayList<GraphViewSeries> eegSeries = new ArrayList<GraphViewSeries>();

	// @Override
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		View view = inflater.inflate(R.layout.impedances, container, false);
		
		LinearLayout layout = (LinearLayout) view;
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT, 1f);
			TextView newText = new TextView(getActivity());
			impedanceList.add(newText);
			newText.setText("Impedance: ");
			newText.setTextSize(16);
			layout.addView(newText, params);
		}
		
		
		// Pad the bottom
		LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT, 2f);
		Space spacer = new Space(getActivity());
		layout.addView(spacer, params);		

		return view;
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState) {
		super.onActivityCreated(savedInstanceState);
		// Allow toggling impedance monitoring
		ToggleButton toggleMonitoring = (ToggleButton) getActivity().findViewById(R.id.monitorImpedance);
		toggleMonitoring.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				ToggleButton toggle = (ToggleButton) v;

				if (objMngr != null) {
					UAVObject obj = objMngr.getObject("EEGSettings");
					if (toggle.isChecked())
						obj.getField("ImpedanceMonitoring").setValue("TRUE");
					else
						obj.getField("ImpedanceMonitoring").setValue("FALSE");
					obj.updated();
				}

			}

		});
	}
	
	@Override
	public void onConnected(UAVObjectManager objMngr) {
		super.onConnected(objMngr);
		UAVObject obj = objMngr.getObject("EEGStatus");
		if (obj != null) {
			obj.updateRequested(); // Make sure this is correct and been updated
			registerObjectUpdates(obj);
			objectUpdated(obj);
		}
		
		obj = objMngr.getObject("EEGSettings");
		if (obj != null) {
			ToggleButton toggleMonitoring = (ToggleButton) getActivity().findViewById(R.id.monitorImpedance);
			boolean monitoring = obj.getField("ImpedanceMonitoring").getValue().toString().compareTo("TRUE") == 0;
			toggleMonitoring.setChecked(monitoring);
		}
	}

	/**
	 * Called whenever any objects subscribed to via registerObjects
	 */
	@Override
	public void objectUpdatedUI(UAVObject obj) {
		if (DEBUG)
			Log.d(TAG, "Updated");

		if (obj == null)
			return;
		
		if (obj.getName().compareTo("EEGStatus") == 0) {
			for (int i = 0; i < HistoryTask.CHANNELS; i++) {
				double impedance = obj.getField("Impedance").getDouble(i);
				impedanceList.get(i).setText(new String("Impedance: ").concat(String.valueOf((int) impedance)).concat(" ohm"));
			}
		}
		
	}

	@Override
	protected String getDebugTag() {
		return TAG;
	}

}
