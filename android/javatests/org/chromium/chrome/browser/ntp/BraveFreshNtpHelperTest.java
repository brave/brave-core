/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.DoNotBatch;
import org.chromium.chrome.browser.app.flags.BraveCachedFlags;
import org.chromium.chrome.browser.app.flags.ChromeCachedFlags;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.cached_flags.BraveCachedFeatureParam;

/** Tests Fresh NTP cached state in a fresh process. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Requires a fresh process for cached-state initialization.")
public class BraveFreshNtpHelperTest {
    @Test
    @SmallTest
    public void testDirectCachedStateAccessBeforeChromeCachedFlagsInitialization() {
        // Access this helper-owned cached flag before initializing ChromeCachedFlags.
        assertNotNull(
                BraveFreshNtpHelper.sBraveFreshNtpAfterIdleExperimentEnabled.getFeatureName());
        // The rewritten ChromeCachedFlags singleton must be BraveCachedFlags.
        assertEquals(BraveCachedFlags.class, ChromeCachedFlags.getInstance().getClass());
        
        assertTrue(
                BraveCachedFeatureParam.sAllBraveInstances.contains(
                        BraveFreshNtpHelper.sBraveFreshNtpAfterIdleExperimentVariant));
    }
}
