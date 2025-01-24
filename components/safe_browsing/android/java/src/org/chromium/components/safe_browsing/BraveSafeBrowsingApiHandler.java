/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.safe_browsing;

import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.webkit.URLUtil;

import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.safetynet.SafeBrowsingThreat;
import com.google.android.gms.safetynet.SafetyNet;
import com.google.android.gms.safetynet.SafetyNetStatusCodes;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.components.safe_browsing.BraveSafeBrowsingUtils.SafeBrowsingJavaResponseStatus;
import org.chromium.components.safe_browsing.BraveSafeBrowsingUtils.SafeBrowsingJavaThreatType;
import org.chromium.components.safe_browsing.SafeBrowsingApiHandler.LookupResult;

import java.util.List;
import java.util.stream.Collectors;

/**
 * Brave implementation of SafeBrowsingApiHandler for Safe Browsing Under the bonnet it still uses
 * SafetyNet.
 */
public class BraveSafeBrowsingApiHandler implements SafeBrowsingApiHandler {
    public static final long SAFE_BROWSING_INIT_INTERVAL_MS = 30000;
    private static final long DEFAULT_CHECK_DELTA = 10;
    private static final String TAG = "BraveSBApiHandler";

    // This is for |threatAttributes| argument at  |SafeBrowsingApiHandler.Observer.onUrlCheckDone|
    // It is used only for UMA Histograms, see ReportSafeBrowsingJavaResponse(); and not returned by
    // SafetyNet API.
    private static final int[] THREAT_ATTRIBUTES_STUB = new int[0];

    /** This is a delegate that is implemented in the object where the handler is created */
    public interface BraveSafeBrowsingApiHandlerDelegate {
        default void turnSafeBrowsingOff() {}

        default boolean isSafeBrowsingEnabled() {
            return true;
        }

        default void maybeShowSafeBrowsingError(String error) {}

        Activity getActivity();
    }

    private Observer mObserver;
    private String mApiKey;
    private boolean mInitialized;
    private int mTriesCount;
    private BraveSafeBrowsingApiHandlerDelegate mBraveSafeBrowsingApiHandlerDelegate;
    private static final Object sLock = new Object();
    private static BraveSafeBrowsingApiHandler sInstance;

    public static BraveSafeBrowsingApiHandler getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveSafeBrowsingApiHandler();
            }
        }
        return sInstance;
    }

    public void setDelegate(String apiKey,
            BraveSafeBrowsingApiHandlerDelegate braveSafeBrowsingApiHandlerDelegate) {
        mApiKey = apiKey;
        mTriesCount = 0;
        mBraveSafeBrowsingApiHandlerDelegate = braveSafeBrowsingApiHandlerDelegate;
        assert mBraveSafeBrowsingApiHandlerDelegate
                != null : "BraveSafeBrowsingApiHandlerDelegate has to be initialized";
    }

    private void resetDelegate() {
        mBraveSafeBrowsingApiHandlerDelegate = null;
    }

    @Override
    public void setObserver(SafeBrowsingApiHandler.Observer observer) {
        mObserver = observer;
    }

    @Override
    public void startUriLookup(long callbackId, String uri, int[] threatTypes, int protocol) {
        if (mBraveSafeBrowsingApiHandlerDelegate == null
                || !mBraveSafeBrowsingApiHandlerDelegate.isSafeBrowsingEnabled()
                || !isHttpsOrHttp(uri)) {
            mObserver.onUrlCheckDone(
                    callbackId,
                    LookupResult.FAILURE_API_CALL_TIMEOUT,
                    SafeBrowsingJavaThreatType.NO_THREAT,
                    THREAT_ATTRIBUTES_STUB,
                    SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
                    DEFAULT_CHECK_DELTA);
            return;
        }

        mTriesCount++;
        if (!mInitialized) {
            initSafeBrowsing();
        }

        SafetyNet.getClient(ContextUtils.getApplicationContext())
                .lookupUri(
                        uri,
                        mApiKey,
                        BraveSafeBrowsingUtils.safeBrowsingToSafetyNetThreatTypes(threatTypes))
                .addOnSuccessListener(
                        mBraveSafeBrowsingApiHandlerDelegate.getActivity(),
                        sbResponse -> {
                            mTriesCount = 0;
                            if (sbResponse.getDetectedThreats().isEmpty()) {
                                mObserver.onUrlCheckDone(
                                        callbackId,
                                        LookupResult.SUCCESS,
                                        SafeBrowsingJavaThreatType.NO_THREAT,
                                        THREAT_ATTRIBUTES_STUB,
                                        SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
                                        DEFAULT_CHECK_DELTA);
                                return;
                            } else {
                                List<Integer> threats =
                                        sbResponse.getDetectedThreats().stream()
                                                .map(SafeBrowsingThreat::getThreatType)
                                                .collect(Collectors.toList());

                                mObserver.onUrlCheckDone(
                                        callbackId,
                                        LookupResult.SUCCESS,
                                        BraveSafeBrowsingUtils
                                                .safetyNetToSafeBrowsingJavaThreatType(
                                                        BraveSafeBrowsingUtils
                                                                .getHighestPriorityThreat(threats)),
                                        THREAT_ATTRIBUTES_STUB,
                                        SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
                                        DEFAULT_CHECK_DELTA);
                                return;
                            }
                        })
                .addOnFailureListener(
                        mBraveSafeBrowsingApiHandlerDelegate.getActivity(),
                        e -> {
                            // An error occurred while communicating with the service.
                            if (e instanceof ApiException) {
                                // An error with the Google Play Services API contains some
                                // additional details.
                                ApiException apiException = (ApiException) e;
                                String error =
                                        "Error: "
                                                + CommonStatusCodes.getStatusCodeString(
                                                        apiException.getStatusCode())
                                                + ", code: "
                                                + apiException.getStatusCode();
                                if (isDebuggable()) {
                                    Log.d(TAG, error);
                                }
                                mBraveSafeBrowsingApiHandlerDelegate.maybeShowSafeBrowsingError(
                                        error);
                                if (apiException.getStatusCode()
                                        == CommonStatusCodes.API_NOT_CONNECTED) {
                                    // That means that device doesn't have Google Play Services API.
                                    // Delegate is used to turn off safe browsing option as every
                                    // request is
                                    // delayed when it's turned on and not working
                                    mBraveSafeBrowsingApiHandlerDelegate.turnSafeBrowsingOff();
                                }

                                // Note: If the status code, apiException.getStatusCode(),
                                // is SafetyNetStatusCodes.SAFE_BROWSING_API_NOT_INITIALIZED,
                                // you need to call initSafeBrowsing(). It means either you
                                // haven't called initSafeBrowsing() before or that it needs
                                // to be called again due to an internal error.
                                if (mTriesCount <= 1
                                        && apiException.getStatusCode()
                                                == SafetyNetStatusCodes
                                                        .SAFE_BROWSING_API_NOT_INITIALIZED) {
                                    initSafeBrowsing();
                                    startUriLookup(callbackId, uri, threatTypes, protocol);
                                } else {
                                    mObserver.onUrlCheckDone(
                                            callbackId,
                                            LookupResult.FAILURE_API_CALL_TIMEOUT,
                                            SafeBrowsingJavaThreatType.NO_THREAT,
                                            THREAT_ATTRIBUTES_STUB,
                                            SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
                                            DEFAULT_CHECK_DELTA);
                                }
                            } else {
                                // A different, unknown type of error occurred.
                                String error = "Error: " + e.getMessage();
                                if (isDebuggable()) {
                                    Log.d(TAG, error);
                                }
                                mBraveSafeBrowsingApiHandlerDelegate.maybeShowSafeBrowsingError(
                                        error);
                                mObserver.onUrlCheckDone(
                                        callbackId,
                                        LookupResult.FAILURE_API_CALL_TIMEOUT,
                                        SafeBrowsingJavaThreatType.NO_THREAT,
                                        THREAT_ATTRIBUTES_STUB,
                                        SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
                                        DEFAULT_CHECK_DELTA);
                            }
                            mTriesCount = 0;
                        });
    }

    public void initSafeBrowsing() {
        SafetyNet.getClient(ContextUtils.getApplicationContext()).initSafeBrowsing();
        mInitialized = true;
    }

    public void shutdownSafeBrowsing() {
        if (!mInitialized) {
            return;
        }
        SafetyNet.getClient(ContextUtils.getApplicationContext()).shutdownSafeBrowsing();
        resetDelegate();
        mInitialized = false;
    }

    private boolean isDebuggable() {
        if (mBraveSafeBrowsingApiHandlerDelegate == null) {
            return false;
        }

        return 0
                != (mBraveSafeBrowsingApiHandlerDelegate.getActivity().getApplicationInfo().flags
                        & ApplicationInfo.FLAG_DEBUGGABLE);
    }

    private boolean isHttpsOrHttp(String uri) {
        return URLUtil.isHttpsUrl(uri) || URLUtil.isHttpUrl(uri);
    }
}
