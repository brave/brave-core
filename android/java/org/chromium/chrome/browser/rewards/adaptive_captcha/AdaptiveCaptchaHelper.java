/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rewards.adaptive_captcha;

import com.google.android.gms.tasks.Task;
import com.google.android.play.core.integrity.IntegrityManager;
import com.google.android.play.core.integrity.IntegrityManagerFactory;
import com.google.android.play.core.integrity.IntegrityTokenRequest;
import com.google.android.play.core.integrity.IntegrityTokenResponse;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.developer.BraveQAPreferences;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.net.ChromiumNetworkAdapter;
import org.chromium.net.NetworkTrafficAnnotationTag;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

public class AdaptiveCaptchaHelper {
    public static final String TAG = "adaptive_captcha";
    private static final String POST_METHOD = "POST";
    private static final String PUT_METHOD = "PUT";

    public static void startAttestation(String captchaId, String paymentId) {
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            HttpURLConnection urlConnection = null;
            String startAttestationUrl = BraveRewardsNativeWorker.getInstance().getAttestationURL();
            if (BraveQAPreferences.shouldVlogRewards()) {
                Log.e(TAG, startAttestationUrl);
            }
            NetworkTrafficAnnotationTag annotation =
                    NetworkTrafficAnnotationTag.createComplete("Brave attestation api android",
                            "semantics {"
                                    + "  sender: 'Brave Android app'"
                                    + "  description: "
                                    + "    'This api gets unique value for payment ID'"
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
                URL url = new URL(startAttestationUrl);
                urlConnection = getUrlConnection(POST_METHOD, url, annotation);

                writeRequestOutput(getStartAttestationBody(paymentId), urlConnection);

                int responseCode = urlConnection.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_CREATED) {
                    JSONObject jsonResponse = new JSONObject(readResponse(urlConnection));
                    if (jsonResponse.has("uniqueValue")) {
                        getPlayIntegrityToken(
                                jsonResponse.optString("uniqueValue"), captchaId, paymentId);
                        logMessage = "Start attestation call is successful";
                    }
                } else {
                    recordFailureAttempt();
                    logMessage = "Start attestation failed with " + responseCode + " : "
                            + urlConnection.getResponseMessage();
                }
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
                if (BraveQAPreferences.shouldVlogRewards()) {
                    Log.e(TAG, logMessage);
                }
            }
        });
    }

    private static void getPlayIntegrityToken(String nonce, String captchaId, String paymentId) {
        IntegrityManager integrityManager =
                IntegrityManagerFactory.create(ContextUtils.getApplicationContext());
        Task<IntegrityTokenResponse> integrityTokenResponseTask =
                integrityManager.requestIntegrityToken(
                        IntegrityTokenRequest.builder().setNonce(nonce).build());
        integrityTokenResponseTask.addOnSuccessListener(
                response -> { attestPaymentId(captchaId, paymentId, response.token(), nonce); });
        integrityTokenResponseTask.addOnFailureListener(e -> {
            Log.e(TAG, "addOnFailureListener : " + e.getMessage());
            recordFailureAttempt();
        });
    }

    private static void attestPaymentId(
            String captchaId, String paymentId, String integrityToken, String uniqueValue) {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    HttpURLConnection urlConnection = null;
                    String attestPaymentIdUrl =
                            BraveRewardsNativeWorker.getInstance()
                                    .getAttestationURLWithPaymentId(paymentId);
                    if (BraveQAPreferences.shouldVlogRewards()) {
                        Log.e(TAG, attestPaymentIdUrl);
                    }
                    NetworkTrafficAnnotationTag annotation =
                            NetworkTrafficAnnotationTag.createComplete(
                                    "Brave attestation api android",
                                    "semantics {  sender: 'Brave Android app'  description:    "
                                        + " 'This api attests play integrity token for the given"
                                        + " payment ID'  trigger: 'When payment id as captcha"
                                        + " scheduled with integrity token'  data:    'integrity"
                                        + " token, unique value, package name'  destination: Brave"
                                        + " grant endpoint}policy {  cookies_allowed: NO "
                                        + " policy_exception_justification: 'Not implemented.'}");
                    String logMessage = "";
                    try {
                        URL url = new URL(attestPaymentIdUrl);
                        urlConnection = getUrlConnection(PUT_METHOD, url, annotation);

                        writeRequestOutput(
                                getAttestPaymentIdBody(integrityToken, uniqueValue), urlConnection);

                        int responseCode = urlConnection.getResponseCode();
                        if (responseCode == HttpURLConnection.HTTP_OK) {
                            solveCaptcha(captchaId, paymentId);
                            logMessage = "Attest payment Id call is successful";
                        } else {
                            logMessage =
                                    "Attest payment Id failed with "
                                            + responseCode
                                            + " : "
                                            + urlConnection.getResponseMessage();
                            recordFailureAttempt();
                        }
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage());
                    } finally {
                        if (urlConnection != null) urlConnection.disconnect();
                        if (BraveQAPreferences.shouldVlogRewards()) {
                            Log.e(TAG, logMessage);
                        }
                    }
                });
    }

    private static void solveCaptcha(String captchaId, String paymentId) {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    HttpURLConnection urlConnection = null;
                    String solveCaptchaUrl =
                            BraveRewardsNativeWorker.getInstance()
                                    .getCaptchaSolutionURL(paymentId, captchaId);
                    if (BraveQAPreferences.shouldVlogRewards()) {
                        Log.e(TAG, solveCaptchaUrl);
                    }
                    NetworkTrafficAnnotationTag annotation =
                            NetworkTrafficAnnotationTag.createComplete(
                                    "Brave attestation api android",
                                    "semantics {  sender: 'Brave Android app'  description:    "
                                        + " 'This api solves captcha for the given payment ID' "
                                        + " trigger: 'When attestation is successful with the give"
                                        + " integrity token for provided payment id'  data:   "
                                        + " 'payment id'  destination: Brave grant endpoint}policy"
                                        + " {  cookies_allowed: NO  policy_exception_justification:"
                                        + " 'Not implemented.'}");

                    String logMessage = "";
                    try {
                        URL url = new URL(String.format(solveCaptchaUrl, paymentId, captchaId));
                        urlConnection = getUrlConnection(POST_METHOD, url, annotation);
                        urlConnection.connect();

                        writeRequestOutput(getSolveCaptchaBody(paymentId), urlConnection);

                        int responseCode = urlConnection.getResponseCode();
                        if (responseCode == HttpURLConnection.HTTP_OK) {
                            clearCaptchaPrefs();
                            logMessage = "Captcha has been solved.";
                        } else {
                            recordFailureAttempt();
                            logMessage =
                                    "Captcha solution failed with "
                                            + responseCode
                                            + " : "
                                            + urlConnection.getResponseMessage();
                        }
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage());
                    } finally {
                        if (urlConnection != null) urlConnection.disconnect();
                        if (BraveQAPreferences.shouldVlogRewards()) {
                            Log.e(TAG, logMessage);
                        }
                    }
                });
    }

    private static void clearCaptchaPrefs() {
        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .setInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS, 0);
        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .setString(BravePref.SCHEDULED_CAPTCHA_ID, "");
        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .setString(BravePref.SCHEDULED_CAPTCHA_PAYMENT_ID, "");
        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .setBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED, false);
    }

    private static void recordFailureAttempt() {
        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .setInteger(
                        BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS,
                        UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                                        .getInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS)
                                + 1);
    }

    private static HttpURLConnection getUrlConnection(String requestMethod, URL url,
            NetworkTrafficAnnotationTag annotation) throws IOException {
        HttpURLConnection urlConnection =
                (HttpURLConnection) ChromiumNetworkAdapter.openConnection(url, annotation);
        urlConnection.setDoOutput(true);
        urlConnection.setRequestMethod(requestMethod);
        urlConnection.setUseCaches(false);
        urlConnection.setRequestProperty("Content-Type", "application/json");
        return urlConnection;
    }

    private static void writeRequestOutput(String body, HttpURLConnection urlConnection)
            throws IOException {
        OutputStream outputStream = null;
        try {
            outputStream = urlConnection.getOutputStream();
            byte[] input = body.getBytes(StandardCharsets.UTF_8.name());
            outputStream.write(input, 0, input.length);
            outputStream.flush();
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        } finally {
            if (outputStream != null) {
                outputStream.close();
            }
        }
    }

    private static String readResponse(HttpURLConnection urlConnection) throws IOException {
        String responseString = "";
        try (BufferedReader br =
                new BufferedReader(
                        new InputStreamReader(
                                urlConnection.getInputStream(), StandardCharsets.UTF_8.name()))) {
            StringBuilder sb = new StringBuilder();
            String line = null;
            while ((line = br.readLine()) != null) {
                sb.append(line + "\n");
            }
            responseString = sb.toString();
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        return responseString;
    }

    private static String getStartAttestationBody(String paymentId) throws JSONException {
        JSONObject jsonParam = new JSONObject();
        jsonParam.put("paymentId", paymentId);
        return jsonParam.toString();
    }

    private static String getAttestPaymentIdBody(String integrityToken, String uniqueValue)
            throws JSONException {
        JSONObject jsonParam = new JSONObject();
        jsonParam.put("integrityToken", integrityToken);
        jsonParam.put("uniqueValue", uniqueValue);
        jsonParam.put("packageName", ContextUtils.getApplicationContext().getPackageName());
        return jsonParam.toString();
    }

    private static String getSolveCaptchaBody(String paymentId) throws JSONException {
        JSONObject jsonParam = new JSONObject();
        jsonParam.put("solution", paymentId);
        return jsonParam.toString();
    }
}
