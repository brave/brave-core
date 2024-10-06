/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.safe_browsing;

import androidx.annotation.IntDef;

import org.chromium.base.Log;

import java.util.ArrayList;
import java.util.List;

// This class contains utils to convert
// threat type enums between SafeBrowsing api and SafetyNet api
// Is from CR129
// src/components/safe_browsing/android/safe_browsing_api_handler_util.h
//
public class BraveSafeBrowsingUtils {
    private static final String TAG = "BraveSBUtils";

    @IntDef({
        SafetyNetJavaThreatType.UNWANTED_SOFTWARE,
        SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
        SafetyNetJavaThreatType.SOCIAL_ENGINEERING,
        SafetyNetJavaThreatType.SUBRESOURCE_FILTER,
        SafetyNetJavaThreatType.BILLING,
        SafetyNetJavaThreatType.CSD_ALLOWLIST,
        SafetyNetJavaThreatType.MAX_VALUE
    })
    public @interface SafetyNetJavaThreatType {
        int UNWANTED_SOFTWARE = 3;
        int POTENTIALLY_HARMFUL_APPLICATION = 4;
        int SOCIAL_ENGINEERING = 5;
        int SUBRESOURCE_FILTER = 13;
        int BILLING = 15;
        int CSD_ALLOWLIST = 16;
        int MAX_VALUE = 17;
    }

    // SUCCESS_WITH_REAL_TIME
    @IntDef({
        SafeBrowsingJavaResponseStatus.SUCCESS_WITH_LOCAL_BLOCKLIST,
        SafeBrowsingJavaResponseStatus.SUCCESS_WITH_REAL_TIME,
        SafeBrowsingJavaResponseStatus.SUCCESS_FALLBACK_REAL_TIME_TIMEOUT,
        SafeBrowsingJavaResponseStatus.SUCCESS_FALLBACK_REAL_TIME_THROTTLED,
        SafeBrowsingJavaResponseStatus.FAILURE_NETWORK_UNAVAILABLE,
        SafeBrowsingJavaResponseStatus.FAILURE_BLOCK_LIST_UNAVAILABLE,
        SafeBrowsingJavaResponseStatus.FAILURE_INVALID_URL
    })
    public @interface SafeBrowsingJavaResponseStatus {
        int SUCCESS_WITH_LOCAL_BLOCKLIST = 0;
        int SUCCESS_WITH_REAL_TIME = 1;
        int SUCCESS_FALLBACK_REAL_TIME_TIMEOUT = 2;
        int SUCCESS_FALLBACK_REAL_TIME_THROTTLED = 3;
        int FAILURE_NETWORK_UNAVAILABLE = 4;
        int FAILURE_BLOCK_LIST_UNAVAILABLE = 5;
        int FAILURE_INVALID_URL = 6;
    };

    private static int sbThreatTypeToSafetyNetJavaThreatType(int safeBrowsingThreatType) {
        switch (safeBrowsingThreatType) {
            case SBThreatType.BILLING:
                return SafetyNetJavaThreatType.BILLING;
            case SBThreatType.SUBRESOURCE_FILTER:
                return SafetyNetJavaThreatType.SUBRESOURCE_FILTER;
            case SBThreatType.URL_PHISHING:
                return SafetyNetJavaThreatType.SOCIAL_ENGINEERING;
            case SBThreatType.URL_MALWARE:
                return SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION;
            case SBThreatType.URL_UNWANTED:
                return SafetyNetJavaThreatType.UNWANTED_SOFTWARE;
            case SBThreatType.CSD_ALLOWLIST:
                return SafetyNetJavaThreatType.CSD_ALLOWLIST;
            default:
                // Threats codes seen here without SafetyNet counterpart:
                // int SUSPICIOUS_SITE = 20;
                // int SIGNED_IN_SYNC_PASSWORD_REUSE = 15;
                Log.w(
                        TAG,
                        "sbThreatTypeToSafetyNetJavaThreatType: unexpected safeBrowsingThreatType="
                                + safeBrowsingThreatType);
                return SafetyNetJavaThreatType.MAX_VALUE;
        }
    }

    public static int[] safeBrowsingToSafetyNetThreatTypes(int[] safeBrowsingThreatTypes) {
        List<Integer> arrSafetyNetThreatTypes = new ArrayList<Integer>();
        for (int i = 0; i < safeBrowsingThreatTypes.length; ++i) {
            int safetyNetThreatType =
                    sbThreatTypeToSafetyNetJavaThreatType(safeBrowsingThreatTypes[i]);
            if (safetyNetThreatType != SafetyNetJavaThreatType.MAX_VALUE) {
                arrSafetyNetThreatTypes.add(safetyNetThreatType);
            }
        }
        return arrSafetyNetThreatTypes.stream().mapToInt(i -> i).toArray();
    }

    public static int safetyNetJavaThreatTypeToSBThreatType(int safetyNetThreatType) {
        switch (safetyNetThreatType) {
            case SafetyNetJavaThreatType.BILLING:
                return SBThreatType.BILLING;
            case SafetyNetJavaThreatType.SUBRESOURCE_FILTER:
                return SBThreatType.SUBRESOURCE_FILTER;
            case SafetyNetJavaThreatType.SOCIAL_ENGINEERING:
                return SBThreatType.URL_PHISHING;
            case SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION:
                return SBThreatType.URL_MALWARE;
            case SafetyNetJavaThreatType.UNWANTED_SOFTWARE:
                return SBThreatType.URL_UNWANTED;
            case SafetyNetJavaThreatType.CSD_ALLOWLIST:
                return SBThreatType.CSD_ALLOWLIST;
            default:
                Log.w(
                        TAG,
                        "safetyNetJavaThreatTypeToSBThreatType: unexpected safetyNetThreatType="
                                + safetyNetThreatType);
                return SafetyNetJavaThreatType.MAX_VALUE;
        }
    }
}
