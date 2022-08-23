/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;
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

public class BraveAdaptiveCaptchaUtils {
    private static final String TAG = "brave_adaptive_captcha_android";
    public static void solveCaptcha(String captchaId, String paymentId) {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        executor.execute(() -> {
            HttpURLConnection urlConnection = null;
            String adaptiveCaptchaSolutionUrl =
                    BraveRewardsNativeWorker.getInstance().getCaptchaSolutionURL(
                            paymentId, captchaId);
            Log.e(TAG, adaptiveCaptchaSolutionUrl);
            NetworkTrafficAnnotationTag annotation = NetworkTrafficAnnotationTag.createComplete(
                    "Brave adaptive captcha solution android",
                    "semantics {"
                            + "  sender: 'Brave Android app'"
                            + "  description: "
                            + "    'This api solves the scheduled captcha for to refill unblinded tokens '"
                            + "  trigger: 'When payment id as captcha scheduled'"
                            + "  data:"
                            + "    'payment id'"
                            + "  destination: Brave grant endpoint"
                            + "}"
                            + "policy {"
                            + "  cookies_allowed: NO"
                            + "  policy_exception_justification: 'Not implemented.'"
                            + "}");

            String logMessage = "";
            try {
                URL url = new URL(String.format(adaptiveCaptchaSolutionUrl, paymentId, captchaId));
                urlConnection =
                        (HttpURLConnection) ChromiumNetworkAdapter.openConnection(url, annotation);
                urlConnection.setDoOutput(true);
                urlConnection.setRequestMethod("POST");
                urlConnection.setUseCaches(false);
                urlConnection.setRequestProperty("Content-Type", "application/json");
                urlConnection.connect();

                JSONObject jsonParam = new JSONObject();
                jsonParam.put("solution", paymentId);

                OutputStream outputStream = urlConnection.getOutputStream();
                byte[] input = jsonParam.toString().getBytes(StandardCharsets.UTF_8.name());
                outputStream.write(input, 0, input.length);
                outputStream.flush();
                outputStream.close();

                int responseCode = urlConnection.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    BufferedReader br = new BufferedReader(new InputStreamReader(
                            urlConnection.getInputStream(), StandardCharsets.UTF_8.name()));
                    StringBuilder sb = new StringBuilder();
                    String line = null;
                    while ((line = br.readLine()) != null) {
                        sb.append(line + "\n");
                    }
                    clearCaptchaPrefs();
                    logMessage = "Captcha has been solved.";
                    br.close();
                } else {
                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .setInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS,
                                    UserPrefs.get(Profile.getLastUsedRegularProfile())
                                                    .getInteger(
                                                            BravePref
                                                                    .SCHEDULED_CAPTCHA_FAILED_ATTEMPTS)
                                            + 1);
                    logMessage = "Captcha solution failed with " + responseCode + " : "
                            + urlConnection.getResponseMessage();
                }
            } catch (MalformedURLException e) {
                Log.e(TAG, e.getMessage());
            } catch (IOException e) {
                Log.e(TAG, e.getMessage());
            } catch (JSONException e) {
                Log.e(TAG, e.getMessage());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
                Log.e(TAG, logMessage);
            }
        });
    }

    private static void clearCaptchaPrefs() {
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS, 0);
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(BravePref.SCHEDULED_CAPTCHA_ID, "");
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(BravePref.SCHEDULED_CAPTCHA_PAYMENT_ID, "");
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED, false);
    }
}
