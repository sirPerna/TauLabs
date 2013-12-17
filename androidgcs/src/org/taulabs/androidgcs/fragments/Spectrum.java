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

public class Spectrum extends ObjectManagerFragment {

	private static final String TAG = Spectrum.class.getSimpleName();
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
		GraphViewDataInterface eegData[][] = new GraphViewDataInterface[HistoryTask.CHANNELS][HistoryTask.HISTORY_LEN / 2];
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			// Only plot positive half of spectrum
			for (int j = 0; j < HistoryTask.HISTORY_LEN / 2; j++) {
				eegData[i][j] = new DataInterface(i,j);
			}
			
			// Create a data series consisting of these data interfaces
			eegSeries.add(new GraphViewSeries(eegData[i]));
		}
	}
		
	Thread timer;
	
	float spectrum[][] = new float[HistoryTask.CHANNELS][HistoryTask.HISTORY_LEN];
	
	private class DataInterface implements GraphViewDataInterface {

		int idx;
		int channel = 0;
		
		public DataInterface(int channel, int idx) {
			this.idx = idx;
		}
		
		@Override
		public double getX() {
			return idx;
		}

		@Override
		public double getY() {
			return spectrum[channel][idx];
		}
		
	}
	
	private Handler uiCallback = new Handler () {
	    public void handleMessage (Message msg) {

	    	// Force a redraw which will look up the fresh data
	    	for (int i = 0; i < HistoryTask.CHANNELS; i++) {
	    		graphView.get(i).removeAllSeries();
	    		graphView.get(i).addSeries(eegSeries.get(i));
	    	}
	    }
	};
	
	private boolean stopTimer;
	
	public void onResume() {
		super.onResume();
	
		// Add graphs to layout
		LinearLayout layout = (LinearLayout) getActivity().findViewById(R.id.eegPlotLayout);
		for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			graphView.add(new LineGraphView(getActivity(), "EEG Data: " + i));
			graphView.get(i).setTitle("");
			graphView.get(i).getGraphViewStyle().setNumVerticalLabels(0);
			graphView.get(i).getGraphViewStyle().setNumHorizontalLabels(0);
			graphView.get(i).getGraphViewStyle().setTextSize(0);

			LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1);
			layout.addView(graphView.get(i), params);
		}
		
		stopTimer = false;
		timer = new Thread() {
		    public void run () {

		    	FFT f = new FFT(HistoryTask.HISTORY_LEN);		    	
		    	float dataTemp[] = new float[HistoryTask.HISTORY_LEN];
		    	float spectrumTemp[] = new float[HistoryTask.HISTORY_LEN];
		    	
		    	String plotMsg = new String();
		    	
		        while (!stopTimer) {

		        	if (history != null) {
			        	for (int i = 0; i < HistoryTask.CHANNELS; i++) {
			        		System.arraycopy(history.getHistory()[i], 0, dataTemp, 0, HistoryTask.HISTORY_LEN);
			        		System.arraycopy(history.getHistory()[i], 0, spectrumTemp, 0, HistoryTask.HISTORY_LEN);
			        		f.fft(dataTemp, spectrumTemp);
			        		
			        		for (int j = 0; j < HistoryTask.HISTORY_LEN; j++) {
			        			spectrumTemp[j] = (float) Math.sqrt((double) (spectrumTemp[j]*spectrumTemp[j] + 
			        					dataTemp[j] * dataTemp[j]));
			        		}
			        		System.arraycopy(spectrumTemp, 0, spectrum[i], 0, HistoryTask.HISTORY_LEN);
			        	}
	
			        	uiCallback.sendEmptyMessage(0);

		        	}
		            try {
						Thread.sleep(100); // Update at 10 Hz
					} catch (InterruptedException e) {
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

class FFT {

	  int n, m;

	  // Lookup tables. Only need to recompute when size of FFT changes.
	  float[] cos;
	  float[] sin;

	  public FFT(int n) {
	      this.n = n;
	      this.m = (int) (Math.log(n) / Math.log(2));

	      // Make sure n is a power of 2
	      if (n != (1 << m))
	          throw new RuntimeException("FFT length must be power of 2");

	      // precompute tables
	      cos = new float[n / 2];
	      sin = new float[n / 2];

	      for (int i = 0; i < n / 2; i++) {
	          cos[i] = (float) Math.cos(-2 * Math.PI * i / n);
	          sin[i] = (float) Math.sin(-2 * Math.PI * i / n);
	      }

	  }

	  public void fft(float[] x, float[] y) {
	      int i, j, k, n1, n2, a;
	      float c, s, t1, t2;

	      // Bit-reverse
	      j = 0;
	      n2 = n / 2;
	      for (i = 1; i < n - 1; i++) {
	          n1 = n2;
	          while (j >= n1) {
	              j = j - n1;
	              n1 = n1 / 2;
	          }
	          j = j + n1;

	          if (i < j) {
	              t1 = x[i];
	              x[i] = x[j];
	              x[j] = t1;
	              t1 = y[i];
	              y[i] = y[j];
	              y[j] = t1;
	          }
	      }

	      // FFT
	      n1 = 0;
	      n2 = 1;

	      for (i = 0; i < m; i++) {
	          n1 = n2;
	          n2 = n2 + n2;
	          a = 0;

	          for (j = 0; j < n1; j++) {
	              c = cos[a];
	              s = sin[a];
	              a += 1 << (m - i - 1);

	              for (k = j; k < n; k = k + n2) {
	                  t1 = c * x[k + n1] - s * y[k + n1];
	                  t2 = s * x[k + n1] + c * y[k + n1];
	                  x[k + n1] = x[k] - t1;
	                  y[k + n1] = y[k] - t2;
	                  x[k] = x[k] + t1;
	                  y[k] = y[k] + t2;
	              }
	          }
	      }
	  }
	}