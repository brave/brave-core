/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.content.SharedPreferences;
import android.util.Base64;

import androidx.annotation.NonNull;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.safetynet.SafetyNet;
import com.google.android.gms.safetynet.SafetyNetApi;
import com.google.android.gms.safetynet.SafetyNetClient;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;

import java.security.SecureRandom;
import java.util.Calendar;
import java.util.Random;
import java.util.regex.Pattern;

/**
 * Utility class for providing additional safety checks.
 */
@JNINamespace("safetynet_check")
public class SafetyNetCheck {
    private static final String TAG = "SafetyNetCheck";

    private static final String PREF_SAFETYNET_RESULT = "safetynet_result";
    private static final String PREF_SAFETYNET_LAST_TIME_CHECK = "safetynet_last_time_check";

    private static final String SAFETYNET_STATUS_VERIFIED_PASSED = "verified, passed";
    private static final String SAFETYNET_STATUS_VERIFIED_NOT_PASSED = "verified, not passed";
    private static final String SAFETYNET_STATUS_NOT_VERIFIED = "not verified";

    private static final long TEN_DAYS = 10 * 24 * 60 * 60 * 1000;

    private long mNativeSafetyNetCheck;
    private Callback<Boolean> mSafetyNetCheckCallback;

    private SafetyNetCheck(long staticSafetyNetCheck) {
        mNativeSafetyNetCheck = staticSafetyNetCheck;
        mSafetyNetCheckCallback = null;
    }

    private SafetyNetCheck(Callback<Boolean> safetyNetCheckCallback) {
        mNativeSafetyNetCheck = 0;
        mSafetyNetCheckCallback = safetyNetCheckCallback;
    }

    @CalledByNative
    private static SafetyNetCheck create(long staticSafetyNetCheck) {
        return new SafetyNetCheck(staticSafetyNetCheck);
    }

    @CalledByNative
    private void destroy() {
        assert mNativeSafetyNetCheck != 0;
        mNativeSafetyNetCheck = 0;
    }

    /**
    * Performs client attestation
    */
    @CalledByNative
    public boolean clientAttestation(String nonceData, String apiKey,
            boolean performAttestationOnClient) {
        return clientAttestation(nonceData, apiKey, performAttestationOnClient, false);
    }

    private boolean clientAttestation(String nonceData, String apiKey,
            boolean performAttestationOnClient, boolean forceCheck) {
        boolean res = false;
        try {
            Activity activity = (Activity)BraveRewardsHelper.getChromeTabbedActivity();
            if (activity == null) return false;
            if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(activity) == ConnectionResult.SUCCESS) {
                SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
                String safetyNetResult = sharedPreferences.getString(PREF_SAFETYNET_RESULT, "");
                long lastTimeCheck = sharedPreferences.getLong(PREF_SAFETYNET_LAST_TIME_CHECK, 0);
                Calendar currentTime = Calendar.getInstance();
                long milliSeconds = currentTime.getTimeInMillis();
                if (!forceCheck && nonceData.isEmpty() && !safetyNetResult.isEmpty()
                        && (milliSeconds - lastTimeCheck < TEN_DAYS)) {
                    clientAttestationResult(true, safetyNetResult, performAttestationOnClient);
                    return true;
                }
                byte[] nonce = nonceData.isEmpty() ? getRequestNonce() : nonceData.getBytes();
                SafetyNetClient client = SafetyNet.getClient(activity);
                Task<SafetyNetApi.AttestationResponse> attestTask = client.attest(nonce, apiKey);
                attestTask.addOnSuccessListener(activity,
                    new OnSuccessListener<SafetyNetApi.AttestationResponse>() {
                        @Override
                        public void onSuccess(SafetyNetApi.AttestationResponse response) {
                            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
                            sharedPreferencesEditor.putString(PREF_SAFETYNET_RESULT, response.getJwsResult());
                            sharedPreferencesEditor.putLong(PREF_SAFETYNET_LAST_TIME_CHECK, milliSeconds);
                            sharedPreferencesEditor.apply();
                            clientAttestationResult(true, response.getJwsResult(), performAttestationOnClient);
                        }
                    }).addOnFailureListener(activity, new OnFailureListener() {
                        @Override
                        public void onFailure(@NonNull Exception e) {
                            Log.e(TAG, "Failed to perform SafetyNetCheck: " + e);
                            clientAttestationResult(false, e.toString(), performAttestationOnClient);
                        }
                    });
                res = true;
            } else {
                clientAttestationResult(false, "Google Play Services are not available", performAttestationOnClient);
                // This result just indicates that callback with the actual result was called
                res = true;
            }
        } catch (Exception e) {
            Log.e(TAG, "SafetyNetCheck error: " + e);
        }

        return res;
    }

    /**
    * Generates a random 24-byte nonce.
    */
    private byte[] getRequestNonce() {
        byte[] bytes = new byte[24];
        Random random = new SecureRandom();
        random.nextBytes(bytes);
        return bytes;
    }

    /**
    * Returns client attestation final result
    */
    private void clientAttestationResult(
            boolean tokenReceived, String resultString, boolean performAttestationOnClient) {
        boolean attestationPassed = false;
        if (performAttestationOnClient) {
            if (tokenReceived) {
                String[] tokenParts = resultString.split(Pattern.quote("."));
                if (tokenParts.length >= 2) {
                    String tokenPart2 = new String(Base64.decode(tokenParts[1], Base64.DEFAULT));
                    boolean ctsProfileMatch = false;
                    boolean basicIntegrity = false;
                    try {
                        JSONObject json = new JSONObject(tokenPart2);
                        if (json.has("ctsProfileMatch")) {
                            ctsProfileMatch = json.getBoolean("ctsProfileMatch");
                        }
                        if (json.has("basicIntegrity")) {
                            basicIntegrity = json.getBoolean("basicIntegrity");
                        }
                    } catch (JSONException e) {
                        Log.e(TAG, "Unable to perform SafetyNet attestation: " + e);
                    }
                    attestationPassed = ctsProfileMatch && basicIntegrity;
                    BravePrefServiceBridge.getInstance().setSafetynetStatus(attestationPassed
                                    ? SAFETYNET_STATUS_VERIFIED_PASSED
                                    : SAFETYNET_STATUS_VERIFIED_NOT_PASSED);
                    if (mSafetyNetCheckCallback != null) {
                        mSafetyNetCheckCallback.onResult(attestationPassed);
                    }
                }
            } else {
                BravePrefServiceBridge.getInstance().setSafetynetStatus(
                        SAFETYNET_STATUS_NOT_VERIFIED);
            }
        }
        if (mNativeSafetyNetCheck == 0) return;
        nativeclientAttestationResult(
                mNativeSafetyNetCheck, tokenReceived, resultString, attestationPassed);
    }

    public String getApiKey() {
        return nativeGetApiKey();
    }

    public static boolean updateSafetynetStatus(Callback<Boolean> safetyNetCheckCallback) {
        SafetyNetCheck safetyNet = new SafetyNetCheck(safetyNetCheckCallback);
        return safetyNet.clientAttestation("", safetyNet.getApiKey(), true, true);
    }

    private native void nativeclientAttestationResult(long nativeSafetyNetCheck,
            boolean tokenReceived, String resultString, boolean attestationPassed);
    private native String nativeGetApiKey();
}
