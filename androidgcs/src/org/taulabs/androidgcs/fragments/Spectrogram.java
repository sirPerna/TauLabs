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


import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class Spectrogram extends ObjectManagerFragment {

	private static final String TAG = Spectrogram.class.getSimpleName();
	private static final int LOGLEVEL = 0;
	// private static boolean WARN = LOGLEVEL > 1;
	private static final boolean DEBUG = LOGLEVEL > 0;

	private static final int SPECTRUM_NUM = 512; 
	private HistoryTask history;
	float spectrogram[][] = new float[HistoryTask.HISTORY_LEN / 2][SPECTRUM_NUM];

	private Bitmap bitmap;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		bitmap = Bitmap.createBitmap(HistoryTask.HISTORY_LEN / 2, SPECTRUM_NUM, Bitmap.Config.ARGB_8888);
		return inflater.inflate(R.layout.spectrogram, container, false);
	}

	@Override
	public void onConnected(UAVObjectManager objMngr) {
		super.onConnected(objMngr);
		history = ((ObjectManagerActivity) getActivity()).getHistoryTask();
	}
			
	private Handler uiCallback = new Handler () {
	    public void handleMessage (Message msg) {

	    	// TODO: redraw the image 
	    	ImageView im = (ImageView) getActivity().findViewById(R.id.imageSpectrogram);
	    	float min = 1e10f;
	    	float max = -1e10f;
	    	
	    	for (int i = 0; i < HistoryTask.HISTORY_LEN / 2; i++)
	    		for (int j = 0; j < SPECTRUM_NUM; j++) {
	    			if (spectrogram[i][j] < min)
	    				min = spectrogram[i][j];
	    			if (spectrogram[i][j] > max)
	    				max = spectrogram[i][j];
	    		}

	    	for (int i = 0; i < HistoryTask.HISTORY_LEN / 2; i++)
	    		for (int j = 0; j < SPECTRUM_NUM; j++) {
	    			float val = (spectrogram[i][j] - min) / (max - min); // scale numbers from 0 to 1
	    			val = (val > 1) ? 1 : 
	    				  (val < 0) ? 0 :
	    				   val;
	    			int col = Color.rgb((int) (val*255), (int) (val*255), (int) (255-val*255));
	    			
	    			bitmap.setPixel(i, j, col);
	    		}
	    	im.setImageBitmap(bitmap);
	    	im.setBackgroundColor(Color.BLACK);

	    }
	};
	
	private Thread timer;
	private boolean stopTimer;
	
	public void onResume() {
		super.onResume();
	
		stopTimer = false;
		timer = new Thread() {
		    public void run () {
		    	
		    	int channelIdx = 0;
		    	int count = 0;

		    	FFT f = new FFT(HistoryTask.HISTORY_LEN);		    	
		    	float dataTemp[] = new float[HistoryTask.HISTORY_LEN];
		    	float spectrumTemp[] = new float[HistoryTask.HISTORY_LEN];
		    	
		        while (!stopTimer) {

		        	if (history != null) {
		        		count++;
		        		
		        		System.arraycopy(history.getHistory()[channelIdx], 0, dataTemp, 0, HistoryTask.HISTORY_LEN);
		        		System.arraycopy(history.getHistory()[channelIdx], 0, spectrumTemp, 0, HistoryTask.HISTORY_LEN);
		        				        		
		        		f.fft(dataTemp, spectrumTemp);

		        		// Zero the DC component
		        		spectrumTemp[0] = 0;
		        		dataTemp[0] = 0;
		        		
		        		for (int j = 0; j < HistoryTask.HISTORY_LEN; j++) {
		        			spectrumTemp[j] = (float) Math.sqrt((double) (spectrumTemp[j]*spectrumTemp[j] + 
		        					dataTemp[j] * dataTemp[j]));
		        		}
		        		
		        		for (int i = 0; i < SPECTRUM_NUM-1; i++) {
		        			System.arraycopy(spectrogram[i+1], 0, spectrogram[i], 0, HistoryTask.HISTORY_LEN / 2);
		        		}
		        		System.arraycopy(spectrumTemp, 0, spectrogram[SPECTRUM_NUM-1], 0, HistoryTask.HISTORY_LEN / 2);

		        		if (count % 10 == 0)
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

	@Override
	protected String getDebugTag() {
		return TAG;
	}

}

