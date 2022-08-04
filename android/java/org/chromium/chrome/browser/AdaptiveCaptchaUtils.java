/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AdaptiveCaptchaUtils {
    private static final String TAG = "adaptive_captcha_android";
    private static final String ADAPTIVE_CAPTCHA_URL =
            "http://url/v3/captcha/solution/%s/%s";

    public static synchronized void solveCaptcha(String captchaId, String paymentId) {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        executor.execute(() -> {
            HttpURLConnection urlConnection = null;
            try {
                URL url = new URL(String.format(ADAPTIVE_CAPTCHA_URL, paymentId, captchaId));
                Log.e(TAG, url.toString());
                urlConnection = (HttpURLConnection) ChromiumNetworkAdapter.openConnection(
                        url, NetworkTrafficAnnotationTag.MISSING_TRAFFIC_ANNOTATION);
                urlConnection.setDoOutput(true);
                urlConnection.setRequestMethod("POST");
                urlConnection.setUseCaches(false);
                urlConnection.setRequestProperty("Content-Type", "application/json");
                urlConnection.setRequestProperty(
                        "Authorization", "Bearer token");
                urlConnection.connect();

                JSONObject jsonParam = new JSONObject();
                jsonParam.put("solution", paymentId);

                OutputStream outputStream = urlConnection.getOutputStream();
                byte[] input = jsonParam.toString().getBytes(StandardCharsets.UTF_8.name());
                outputStream.write(input, 0, input.length);
                outputStream.flush();
                outputStream.close();

                int HttpResult = urlConnection.getResponseCode();
                if (HttpResult == HttpURLConnection.HTTP_OK) {
                    BufferedReader br = new BufferedReader(new InputStreamReader(
                            urlConnection.getInputStream(), StandardCharsets.UTF_8.name()));
                    StringBuilder sb = new StringBuilder();
                    String line = null;
                    while ((line = br.readLine()) != null) {
                        sb.append(line + "\n");
                    }
                    Log.e(TAG, line);
                    br.close();
                } else {
                    BravePrefServiceBridge.getInstance().incrementFailedAttempts();
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
        });
    }
}
