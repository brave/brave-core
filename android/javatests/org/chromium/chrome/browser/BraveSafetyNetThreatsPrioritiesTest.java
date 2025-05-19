/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.safe_browsing.BraveSafeBrowsingUtils;
import org.chromium.components.safe_browsing.BraveSafeBrowsingUtils.SafetyNetJavaThreatType;

import java.util.ArrayList;
import java.util.Arrays;

@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveSafetyNetThreatsPrioritiesTest {

    @Test
    @SmallTest
    public void testPriorities() throws Exception {
        // Fail safe option for empty input, consider it is safe
        Assert.assertEquals(
                SafetyNetJavaThreatType.MAX_VALUE,
                BraveSafeBrowsingUtils.getHighestPriorityThreat(new ArrayList<>()));

        // Single input must return the same value
        Assert.assertEquals(
                SafetyNetJavaThreatType.SOCIAL_ENGINEERING,
                BraveSafeBrowsingUtils.getHighestPriorityThreat(
                        Arrays.asList(new Integer[] {SafetyNetJavaThreatType.SOCIAL_ENGINEERING})));

        // Several threats as input - return the most priority one
        Assert.assertEquals(
                SafetyNetJavaThreatType.SOCIAL_ENGINEERING,
                BraveSafeBrowsingUtils.getHighestPriorityThreat(
                        Arrays.asList(
                                new Integer[] {
                                    SafetyNetJavaThreatType.UNWANTED_SOFTWARE,
                                    SafetyNetJavaThreatType.SUBRESOURCE_FILTER,
                                    SafetyNetJavaThreatType.SOCIAL_ENGINEERING,
                                    SafetyNetJavaThreatType.BILLING,
                                    SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
                                })));

        Assert.assertEquals(
                SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
                BraveSafeBrowsingUtils.getHighestPriorityThreat(
                        Arrays.asList(
                                new Integer[] {
                                    SafetyNetJavaThreatType.SUBRESOURCE_FILTER,
                                    SafetyNetJavaThreatType.CSD_ALLOWLIST,
                                    SafetyNetJavaThreatType.POTENTIALLY_HARMFUL_APPLICATION,
                                    SafetyNetJavaThreatType.UNWANTED_SOFTWARE,
                                })));
    }
}
