/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.safe_browsing;

import androidx.annotation.IntDef;
import androidx.annotation.VisibleForTesting;

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

    @IntDef({
        SafeBrowsingJavaThreatType.NO_THREAT,
        SafeBrowsingJavaThreatType.SOCIAL_ENGINEERING,
        SafeBrowsingJavaThreatType.UNWANTED_SOFTWARE,
        SafeBrowsingJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
        SafeBrowsingJavaThreatType.BILLING,
        SafeBrowsingJavaThreatType.ABUSIVE_EXPERIENCE_VIOLATION,
        SafeBrowsingJavaThreatType.BETTER_ADS_VIOLATION
    })
    public @interface SafeBrowsingJavaThreatType {
        int NO_THREAT = 0;
        int SOCIAL_ENGINEERING = 2;
        int UNWANTED_SOFTWARE = 3;
        int POTENTIALLY_HARMFUL_APPLICATION = 4;
        int BILLING = 15;
        int ABUSIVE_EXPERIENCE_VIOLATION = 20;
        int BETTER_ADS_VIOLATION = 21;
    };

    private static int safeBrowsingToSafetyNetJavaThreatType(int safeBrowsingThreatType) {
        switch (safeBrowsingThreatType) {
            case SafeBrowsingJavaThreatType.NO_THREAT:
                return 0;
            case SafeBrowsingJavaThreatType.SOCIAL_ENGINEERING:
                return SafetyNetJavaThreatType.SOCIAL_ENGINEERING;
            case SafeBrowsingJavaThreatType.UNWANTED_SOFTWARE:
                return SafetyNetJavaThreatType.UNWANTED_SOFTWARE;
            case SafeBrowsingJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION:
                return SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION;
            case SafeBrowsingJavaThreatType.BILLING:
                return SafetyNetJavaThreatType.BILLING;
            case SafeBrowsingJavaThreatType.ABUSIVE_EXPERIENCE_VIOLATION:
            case SafeBrowsingJavaThreatType.BETTER_ADS_VIOLATION:
                // See SafeBrowsingJavaToSBThreatType at safe_browsing_api_handler_bridge.cc
                return SafetyNetJavaThreatType.SUBRESOURCE_FILTER;
            default:
                Log.d(
                        TAG,
                        "safeBrowsingToSafetyNetJavaThreatType: unexpected safeBrowsingThreatType="
                                + safeBrowsingThreatType);
                return SafetyNetJavaThreatType.MAX_VALUE;
        }
    }

    public static int[] safeBrowsingToSafetyNetThreatTypes(int[] safeBrowsingThreatTypes) {
        List<Integer> arrSafetyNetThreatTypes = new ArrayList<Integer>();
        for (int i = 0; i < safeBrowsingThreatTypes.length; ++i) {
            int safetyNetThreatType =
                    safeBrowsingToSafetyNetJavaThreatType(safeBrowsingThreatTypes[i]);
            if (safetyNetThreatType != SafetyNetJavaThreatType.MAX_VALUE) {
                arrSafetyNetThreatTypes.add(safetyNetThreatType);
            }
        }
        return arrSafetyNetThreatTypes.stream().mapToInt(i -> i).toArray();
    }

    public static int safetyNetToSafeBrowsingJavaThreatType(int safetyNetThreatType) {
        switch (safetyNetThreatType) {
            case SafetyNetJavaThreatType.BILLING:
                return SafeBrowsingJavaThreatType.BILLING;
            case SafetyNetJavaThreatType.SUBRESOURCE_FILTER:
                return SafeBrowsingJavaThreatType.NO_THREAT;
            case SafetyNetJavaThreatType.SOCIAL_ENGINEERING:
                return SafeBrowsingJavaThreatType.SOCIAL_ENGINEERING;
            case SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION:
                return SafeBrowsingJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION;
            case SafetyNetJavaThreatType.UNWANTED_SOFTWARE:
                return SafeBrowsingJavaThreatType.UNWANTED_SOFTWARE;
            case SafetyNetJavaThreatType.CSD_ALLOWLIST:
                return SafeBrowsingJavaThreatType.NO_THREAT;
            default:
                Log.d(
                        TAG,
                        "safetyNetToSafeBrowsingJavaThreatType: unexpected safetyNetThreatType="
                                + safetyNetThreatType);
                return SafeBrowsingJavaThreatType.NO_THREAT;
        }
    }

    // Priorities from most high to low
    private static final int SAFETY_NET_THREAT_PRIORITIES[] = {
        SafetyNetJavaThreatType.SOCIAL_ENGINEERING,
        SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
        SafetyNetJavaThreatType.BILLING,
        SafetyNetJavaThreatType.UNWANTED_SOFTWARE,
        SafetyNetJavaThreatType.SUBRESOURCE_FILTER,
        SafetyNetJavaThreatType.CSD_ALLOWLIST,
        SafetyNetJavaThreatType.MAX_VALUE
    };

    @VisibleForTesting
    public static int getHighestPriorityThreat(List<Integer> threats) {
        if (threats.size() == 0) {
            return SafetyNetJavaThreatType.MAX_VALUE;
        }

        if (threats.size() == 1) {
            return threats.get(0);
        }

        for (int i = 0; i < SAFETY_NET_THREAT_PRIORITIES.length; ++i) {
            if (threats.contains(SAFETY_NET_THREAT_PRIORITIES[i])) {
                return SAFETY_NET_THREAT_PRIORITIES[i];
            }
        }

        assert false : "Unexpected threat";
        return SafetyNetJavaThreatType.MAX_VALUE;
    }
}
