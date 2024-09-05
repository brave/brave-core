/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.components.version_info.BraveVersionConstants;
import org.chromium.net.ChromiumNetworkAdapter;
import org.chromium.net.NetworkTrafficAnnotationTag;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;

public class BraveShieldsUtils {
    private static final String TAG = "Shields";
    public static final String PREF_SHIELDS_TOOLTIP = "shields_tooltip";
    public static boolean isTooltipShown;
    public static final String WEBCOMPAT_UI_SOURCE_HISTOGRAM_NAME = "Brave.Webcompat.UISource";
    public static final String WEBCOMPAT_REPORT_BRAVE_VERSION = "version";
    public static final String WEBCOMPAT_REPORT_BRAVE_CHANNEL = "channel";

    public interface BraveShieldsCallback {
        void braveShieldsSubmitted();
    }

        public static boolean hasShieldsTooltipShown(String tooltipType) {
            SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
            return mSharedPreferences.getBoolean(tooltipType, false);
        }

        public static void setShieldsTooltipShown(String tooltipType, boolean isShown) {
            SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(tooltipType, isShown);
            sharedPreferencesEditor.apply();
        }

        public static class BraveShieldsWorkerTask extends AsyncTask<Void> {
            private String mDomain;
            private String mApiKey;

            public BraveShieldsWorkerTask(String domain, String apiKey) {
                mDomain = domain;
                mApiKey = apiKey;
            }

            @Override
            protected Void doInBackground() {
                sendBraveShieldsFeedback(mDomain, mApiKey);
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();

                if (isCancelled()) return;
            }
        }

        private static void sendBraveShieldsFeedback(String domain, String apiKey) {
            assert BraveConfig.WEBCOMPAT_REPORT_ENDPOINT != null
                    && !BraveConfig.WEBCOMPAT_REPORT_ENDPOINT.isEmpty();

        RecordHistogram.recordEnumeratedHistogram(WEBCOMPAT_UI_SOURCE_HISTOGRAM_NAME, 0, 2);

        Context context = ContextUtils.getApplicationContext();
        StringBuilder sb = new StringBuilder();

        HttpURLConnection urlConnection = null;
        try {
            URL url = new URL(BraveConfig.WEBCOMPAT_REPORT_ENDPOINT);
            urlConnection =
                    (HttpURLConnection)
                            ChromiumNetworkAdapter.openConnection(
                                    url, NetworkTrafficAnnotationTag.MISSING_TRAFFIC_ANNOTATION);
            urlConnection.setDoOutput(true);
            urlConnection.setRequestMethod("POST");
            urlConnection.setUseCaches(false);
            urlConnection.setRequestProperty("Content-Type", "application/json");
            urlConnection.connect();

            JSONObject jsonParam = new JSONObject();
            jsonParam.put("domain", domain);
            jsonParam.put("api_key", apiKey);
            jsonParam.put(WEBCOMPAT_REPORT_BRAVE_VERSION, BraveVersionConstants.VERSION);
            jsonParam.put(WEBCOMPAT_REPORT_BRAVE_CHANNEL, BraveVersionConstants.CHANNEL);

            OutputStream outputStream = urlConnection.getOutputStream();
            byte[] input = jsonParam.toString().getBytes(StandardCharsets.UTF_8.toString());

            outputStream.write(input, 0, input.length);
            outputStream.flush();
            outputStream.close();

            int httpResult = urlConnection.getResponseCode();
            if (httpResult == HttpURLConnection.HTTP_OK) {
                BufferedReader br =
                        new BufferedReader(
                                new InputStreamReader(
                                        urlConnection.getInputStream(),
                                        StandardCharsets.UTF_8.toString()));
                String line = null;
                while ((line = br.readLine()) != null) {
                    sb.append(line + "\n");
                }
                br.close();
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
            if (urlConnection != null) urlConnection.disconnect();
        }
    }
}
