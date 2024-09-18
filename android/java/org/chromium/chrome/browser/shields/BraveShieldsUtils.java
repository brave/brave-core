/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

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
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Random;

public class BraveShieldsUtils {
    private static final String TAG = "Shields";
    public static final String PREF_SHIELDS_TOOLTIP = "shields_tooltip";
    public static boolean isTooltipShown;
    public static final String WEBCOMPAT_UI_SOURCE_HISTOGRAM_NAME = "Brave.Webcompat.UISource";
    public static final String WEBCOMPAT_REPORT_BRAVE_VERSION = "version";
    public static final String WEBCOMPAT_REPORT_BRAVE_CHANNEL = "channel";
    private static final String MULTIPART_CONTENT_TYPE_PREFIX = "multipart/form-data; boundary=%s";
    private static final String MULTIPART_BOUNDARY = "MultipartBoundary";
    private static final int MULTIPART_BOUNDARY_SIZE = 69;
    private static final char[] MULTIPART_CHARS =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ".toCharArray();
    private static final String JSON_CONTENT_TYPE = "application/json";
    private static final String PNG_CONTENT_TYPE = "image/png";
    private static final String REPORT_DETAILS_MULTIPART_NAME = "report-details";
    private static final String SCREENSHOT_MULTIPART_NAME = "screenshot";
    private static final String SCREENSHOT_MULTIPART_FILENAME = "screenshot.png";
    private static final String MULTIPART_LINE_END = "\r\n";

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
        private byte[] mScreenshotBytes;

        public BraveShieldsWorkerTask(String domain, String apiKey, byte[] pngBytes) {
            mDomain = domain;
            mApiKey = apiKey;
            mScreenshotBytes = pngBytes;
        }

        @Override
        protected Void doInBackground() {
            sendBraveShieldsFeedback(mDomain, mApiKey, mScreenshotBytes);
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            assert ThreadUtils.runningOnUiThread();

            if (isCancelled()) return;
        }
    }

    private static void sendBraveShieldsFeedback(
            String domain, String apiKey, byte[] screenshotPngBytes) {
        assert BraveConfig.WEBCOMPAT_REPORT_ENDPOINT != null
                && !BraveConfig.WEBCOMPAT_REPORT_ENDPOINT.isEmpty();

        RecordHistogram.recordEnumeratedHistogram(WEBCOMPAT_UI_SOURCE_HISTOGRAM_NAME, 0, 2);

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

            JSONObject jsonParam = new JSONObject();
            jsonParam.put("domain", domain);
            jsonParam.put("api_key", apiKey);
            jsonParam.put(WEBCOMPAT_REPORT_BRAVE_VERSION, BraveVersionConstants.VERSION);
            jsonParam.put(WEBCOMPAT_REPORT_BRAVE_CHANNEL, BraveVersionConstants.CHANNEL);

            if (!isScreenshotAvailable(screenshotPngBytes)) {
                generateJsonData(urlConnection, jsonParam.toString());
            } else {
                generateMultipartData(urlConnection, jsonParam.toString(), screenshotPngBytes);
            }

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

    private static String generateBoundary() {
        StringBuilder buffer = new StringBuilder();
        buffer.append(String.format("----%s--", MULTIPART_BOUNDARY));
        Random rand = new Random();
        while (buffer.length() < (MULTIPART_BOUNDARY_SIZE - 4)) {
            buffer.append(MULTIPART_CHARS[rand.nextInt(MULTIPART_CHARS.length)]);
        }
        buffer.append("----");
        assert buffer.length() == MULTIPART_BOUNDARY_SIZE;
        return buffer.toString();
    }

    private static byte[] asUtf8Bytes(String val) throws UnsupportedEncodingException {
        return val.getBytes(StandardCharsets.UTF_8.toString());
    }

    private static void generateJsonData(HttpURLConnection urlConnection, String jsonData)
            throws UnsupportedEncodingException, IOException {
        urlConnection.setRequestProperty("Content-Type", "application/json");
        urlConnection.connect();
        try (OutputStream outputStream = urlConnection.getOutputStream()) {
            byte[] input = jsonData.getBytes(StandardCharsets.UTF_8.toString());
            outputStream.write(input, 0, input.length);
            outputStream.flush();
        }
    }

    private static boolean isScreenshotAvailable(byte[] screenshotPngBytes) {
        return screenshotPngBytes != null && screenshotPngBytes.length > 0;
    }

    private static void generateMultipartData(
            HttpURLConnection urlConnection, String jsonData, byte[] screenshotPngBytes)
            throws UnsupportedEncodingException, IOException {
        final String mb = generateBoundary();
        urlConnection.setRequestProperty(
                "Content-Type", String.format(MULTIPART_CONTENT_TYPE_PREFIX, mb));
        urlConnection.connect();

        try (OutputStream os = urlConnection.getOutputStream()) {
            os.write(asUtf8Bytes(String.format("--%s%s", mb, MULTIPART_LINE_END)));
            os.write(
                    asUtf8Bytes(
                            String.format(
                                    "Content-Disposition: form-data; name=\"%s\"%s",
                                    REPORT_DETAILS_MULTIPART_NAME, MULTIPART_LINE_END)));
            os.write(
                    asUtf8Bytes(
                            String.format(
                                    "Content-Type: %s%s", JSON_CONTENT_TYPE, MULTIPART_LINE_END)));
            os.write(
                    asUtf8Bytes(
                            String.format(
                                    "%s%s%s", MULTIPART_LINE_END, jsonData, MULTIPART_LINE_END)));

            os.write(asUtf8Bytes(String.format("--%s%s", mb, MULTIPART_LINE_END)));
            os.write(
                    asUtf8Bytes(
                            String.format(
                                    "Content-Disposition: form-data; name=\"%s\";"
                                            + " filename=\"%s\"%s",
                                    SCREENSHOT_MULTIPART_NAME,
                                    SCREENSHOT_MULTIPART_FILENAME,
                                    MULTIPART_LINE_END)));
            os.write(
                    asUtf8Bytes(
                            String.format(
                                    "Content-Type: %s%s", PNG_CONTENT_TYPE, MULTIPART_LINE_END)));
            os.write(MULTIPART_LINE_END.getBytes(StandardCharsets.UTF_8.toString()));
            os.write(screenshotPngBytes);
            os.write(MULTIPART_LINE_END.getBytes(StandardCharsets.UTF_8.toString()));
            os.write(asUtf8Bytes(String.format("--%s--%s", mb, MULTIPART_LINE_END)));
            os.flush();
        }
    }
}
