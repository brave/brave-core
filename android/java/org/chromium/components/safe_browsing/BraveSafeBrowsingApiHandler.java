/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.safe_browsing;

import android.app.Activity;
import android.content.pm.ApplicationInfo;

import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.safetynet.SafeBrowsingThreat;
import com.google.android.gms.safetynet.SafetyNet;
import com.google.android.gms.safetynet.SafetyNetApi;
import com.google.android.gms.tasks.OnSuccessListener;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;

public class BraveSafeBrowsingApiHandler implements SafeBrowsingApiHandler {
    private static final long DEFAULT_CHECK_DELTA = 10;
    private static final String SAFE_METADATA = "{}";
    private static final String TAG = "BraveSafeBrowsingApiHandler";

    private Observer mObserver;
    private Activity mActivity;
    private String mApiKey;
    private boolean mInitialized;

    public BraveSafeBrowsingApiHandler(Activity activity, String apiKey) {
        mActivity = activity;
        mApiKey = apiKey;
    }

    @Override
    public boolean init(Observer observer) {
        mObserver = observer;
        return true;
    }

    @Override
    public void startUriLookup(final long callbackId, String uri, int[] threatsOfInterest) {
        if (mActivity == null || !mInitialized) {
            return;
        }

        SafetyNet.getClient(ContextUtils.getApplicationContext())
                .lookupUri(uri, mApiKey, threatsOfInterest)
                .addOnSuccessListener(mActivity,
                        sbResponse -> {
                            try {
                                String metadata = SAFE_METADATA;
                                if (!sbResponse.getDetectedThreats().isEmpty()) {
                                    JSONArray jsonArray = new JSONArray();
                                    for (int i = 0; i < sbResponse.getDetectedThreats().size();
                                            i++) {
                                        JSONObject jsonObj = new JSONObject();
                                        jsonObj.put("threat_type",
                                                String.valueOf(sbResponse.getDetectedThreats()
                                                                       .get(i)
                                                                       .getThreatType()));
                                        jsonArray.put(jsonObj);
                                    }
                                    JSONObject finalObj = new JSONObject();
                                    finalObj.put("matches", jsonArray);
                                    metadata = finalObj.toString();
                                }
                                if (mObserver != null) {
                                    mObserver.onUrlCheckDone(callbackId, SafeBrowsingResult.SUCCESS,
                                            metadata, DEFAULT_CHECK_DELTA);
                                }
                            } catch (JSONException e) {
                            }
                        })
                .addOnFailureListener(mActivity, e -> {
                    if (!isDebuggable()) {
                        return;
                    }
                    // An error occurred while communicating with the service.
                    if (e instanceof ApiException) {
                        // An error with the Google Play Services API contains some
                        // additional details.
                        ApiException apiException = (ApiException) e;
                        org.chromium.base.Log.d(TAG,
                                "Error: "
                                        + CommonStatusCodes.getStatusCodeString(
                                                apiException.getStatusCode()));

                        // Note: If the status code, apiException.getStatusCode(),
                        // is SafetyNetStatusCode.SAFE_BROWSING_API_NOT_INITIALIZED,
                        // you need to call initSafeBrowsing(). It means either you
                        // haven't called initSafeBrowsing() before or that it needs
                        // to be called again due to an internal error.
                    } else {
                        // A different, unknown type of error occurred.
                        org.chromium.base.Log.d(TAG, "Error: " + e.getMessage());
                    }
                });
    }

    @Override
    public boolean startAllowlistLookup(final String uri, int threatType) {
        return false;
    }

    public void initSafeBrowsing() {
        SafetyNet.getClient(mActivity).initSafeBrowsing();
        mInitialized = true;
    }

    public void shutdownSafeBrowsing() {
        if (!mInitialized) {
            return;
        }
        SafetyNet.getClient(ContextUtils.getApplicationContext()).shutdownSafeBrowsing();
    }

    private boolean isDebuggable() {
        if (mActivity == null) {
            return false;
        }

        return 0 != (mActivity.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE);
    }
}
