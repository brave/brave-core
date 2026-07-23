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

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;

/**
 * Drives the Android device-attestation captcha flow. The attestation HTTP requests run in native
 * (BraveRewardsNativeWorker); this class only fetches the Play Integrity token, which requires a
 * Java-only Google Play API.
 */
public class AdaptiveCaptchaHelper {
    public static final String TAG = "adaptive_captcha";

    // Entry point: starts the attestation flow in native. Native calls back into
    // fetchPlayIntegrityToken() once the server issues the attestation nonce.
    public static void startAttestation(String captchaId, String paymentId) {
        BraveRewardsNativeWorker worker = BraveRewardsNativeWorker.getInstance();
        if (worker != null) {
            worker.startAttestation(captchaId, paymentId);
        }
    }

    // Called from native with the server-issued nonce. Requests a Play Integrity
    // token for it and hands the result back to native. An empty token signals a
    // failure so native records the attempt.
    public static void fetchPlayIntegrityToken(String captchaId, String paymentId, String nonce) {
        IntegrityManager integrityManager =
                IntegrityManagerFactory.create(ContextUtils.getApplicationContext());
        Task<IntegrityTokenResponse> integrityTokenResponseTask =
                integrityManager.requestIntegrityToken(
                        IntegrityTokenRequest.builder().setNonce(nonce).build());
        integrityTokenResponseTask.addOnSuccessListener(
                response -> attestPaymentId(captchaId, paymentId, response.token(), nonce));
        integrityTokenResponseTask.addOnFailureListener(
                e -> {
                    Log.e(TAG, "Play Integrity token request failed: " + e.getMessage());
                    attestPaymentId(captchaId, paymentId, "", nonce);
                });
    }

    private static void attestPaymentId(
            String captchaId, String paymentId, String integrityToken, String nonce) {
        BraveRewardsNativeWorker worker = BraveRewardsNativeWorker.getInstance();
        if (worker != null) {
            worker.attestPaymentId(
                    captchaId,
                    paymentId,
                    integrityToken,
                    nonce,
                    ContextUtils.getApplicationContext().getPackageName());
        }
    }
}
