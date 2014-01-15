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
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;

public class Graph extends ObjectManagerFragment {

	private static final String TAG = Graph.class.getSimpleName();
	private static final int LOGLEVEL = 0;
	// private static boolean WARN = LOGLEVEL > 1;
	private static final boolean DEBUG = LOGLEVEL > 0;

	private HistoryTask history;
	private ArrayList <GraphView> graphView = new ArrayList<GraphView>();
	private ArrayList <GraphViewSeries> eegSeries = new ArrayList <GraphViewSeries>();

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

		history = ((ObjectManagerActivity) getActivity()).getHistoryTask();
		
		if (history == null)
			Log.e(TAG, "Error: null history");

		// Create array of data objects which index into the history
		GraphViewDataInterface eegData[][] = new GraphViewDataInterface[HistoryTask.CHANNELS][HistoryTask.HISTORY_LEN];
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			for (int j = 0; j < HistoryTask.HISTORY_LEN; j++) {
				eegData[i][j] = new DataInterface(i,j);
			}
			
			// Create a data series consisting of these data interfaces
			eegSeries.add(new GraphViewSeries(eegData[i]));
			graphView.get(i).addSeries(eegSeries.get(i));
		}
	}
		
	Thread timer;
	
	private class DataInterface implements GraphViewDataInterface {

		int idx;
		int channel = 0;
		
		public DataInterface(int channel, int idx) {
			this.idx = idx;
			this.channel = channel;
		}
		
		@Override
		public double getX() {
			return history.getT0() + idx / history.getSamplingRate();
		}

		@Override
		public double getY() {
			return history.getHistory()[channel][idx];
		}
		
	}
	
	private Handler uiCallback = new Handler () {
	    public void handleMessage (Message msg) {

	    	// Force a redraw which will look up the fresh data
	    	for (int i = 0; i < HistoryTask.CHANNELS; i++) {
	    		graphView.get(i).redrawAll();
	    	}
	    }
	};
	
	private boolean stopTimer;

	public void onCreate (Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			graphView.add(new LineGraphView(getActivity(), "EEG Data: " + i));
			graphView.get(i).setTitle("");
			graphView.get(i).getGraphViewStyle().setNumVerticalLabels(0);
			graphView.get(i).getGraphViewStyle().setNumHorizontalLabels(0);
			graphView.get(i).getGraphViewStyle().setTextSize(0);
		}
	}

	public void onResume() {
		super.onResume();
	
		// Add graphs to layout
		LinearLayout layout = (LinearLayout) getActivity().findViewById(R.id.eegPlotLayout);
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1);
			layout.addView(graphView.get(i), params);
		}
		
		stopTimer = false;
		timer = new Thread() {
		    public void run () {
		        while (!stopTimer) {
		        	uiCallback.sendEmptyMessage(0);
		            try {
						Thread.sleep(100); // Update at 10 Hz
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					} 
		        }
		    }
		};
		timer.start();
	}
	
	public void onPause() {
		super.onPause();
		
		// Tell UI thread to stop updating
		stopTimer = true;
		timer.interrupt();
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
	}

	@Override
	protected String getDebugTag() {
		return TAG;
	}

}
