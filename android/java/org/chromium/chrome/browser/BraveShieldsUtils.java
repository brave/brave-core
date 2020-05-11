/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.os.Build;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.OutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.settings.about.AboutChromeSettings;
import org.chromium.chrome.browser.settings.about.AboutSettingsBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;

public class BraveShieldsUtils {
	private static final String TAG = "Shields";

	public interface BraveShieldsCallback {
		void braveShieldsSubmitted();
	}

	public static class BraveShieldsWorkerTask extends AsyncTask<Void> {
		private String mDomain;

	    public BraveShieldsWorkerTask(String domain) {
	    	mDomain = domain;
	    }

	    @Override
	    protected Void doInBackground() {
	        sendBraveShieldsFeedback(mDomain);
	        return null;
	    }

	    @Override
	    protected void onPostExecute(Void result) {
	        assert ThreadUtils.runningOnUiThread();

	        if (isCancelled()) return;
	    }
	}

	private static void sendBraveShieldsFeedback(String domain) {
		Context context = ContextUtils.getApplicationContext();
		StringBuilder sb = new StringBuilder(); 
		String http = "https://laptop-updates.brave.com/1/webcompat";

		HttpURLConnection urlConnection=null;  
		try {  
		    URL url = new URL(http);  
		    urlConnection = (HttpURLConnection) url.openConnection();
		    urlConnection.setDoOutput(true);   
		    urlConnection.setRequestMethod("POST");  
		    urlConnection.setUseCaches(false);  
		    urlConnection.setRequestProperty("Content-Type","application/json");  
		    urlConnection.connect();

            JSONObject jsonParam = new JSONObject();
            jsonParam.put("domain", domain);
            jsonParam.put("api_key", "831c131b-bb19-4953-b77c-e45268b6ef4c");

            Log.e(TAG, jsonParam.toString());

            OutputStream outputStream = urlConnection.getOutputStream();
            byte[] input = jsonParam.toString().getBytes(StandardCharsets.UTF_8.toString());
            outputStream.write(input, 0, input.length);
            outputStream.flush();
            outputStream.close();

		    int HttpResult =urlConnection.getResponseCode();  
		    if(HttpResult ==HttpURLConnection.HTTP_OK) {  
		        BufferedReader br = new BufferedReader(new InputStreamReader(  
		            urlConnection.getInputStream(), StandardCharsets.UTF_8.toString()));  
		        String line = null;  
		        while ((line = br.readLine()) != null) {  
		            sb.append(line + "\n");  
		        }  
		        br.close();
		        Log.e(TAG, sb.toString());
		    } else {  
		    	Log.e(TAG, urlConnection.getResponseMessage());
		    }  
		} catch (MalformedURLException e) {
			Log.e(TAG, e.getMessage());
		} catch (IOException e) {
			Log.e(TAG, e.getMessage());
		} catch (JSONException e) {
		    Log.e(TAG, e.getMessage());
		} finally {  
		    if(urlConnection!=null)  
		    urlConnection.disconnect();  
		}  
	}
}