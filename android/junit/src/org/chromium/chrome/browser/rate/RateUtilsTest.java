/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import static org.junit.Assert.assertFalse;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/**
 * Tests for the "Don't show again" opt-out in {@link RateUtils}.
 *
 * <p>Only the opt-out branch of {@link RateUtils#shouldShowRateDialog} is covered. It returns
 * before {@code mainCriteria()} and {@code anyOneSubCriteria()}, which call into VPN and
 * default-browser statics, so neither the usage-criteria path nor the re-enabling of the prompt
 * after the box is unticked can be asserted here without mocking those.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class RateUtilsTest {
    @Before
    public void setUp() {
        // The key is not registered in the upstream PreferenceKeyRegistry. Production builds
        // tolerate that because BraveStrictPreferenceKeyChecker discards the verdict, but don't
        // rely on that bytecode rewrite being applied to Robolectric targets.
        ChromeSharedPreferences.getInstance().disableKeyCheckerForTesting();
        RateUtils.resetForTesting();
        clearPrefs();
    }

    @After
    public void tearDown() {
        clearPrefs();
    }

    /** Leaves the preference unset rather than written to false, as it is on a fresh install. */
    private void clearPrefs() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_RATE_DONT_SHOW_AGAIN);
    }

    @Test
    public void testShouldShowRateDialog_falseWhenDontShowAgain() {
        RateUtils.getInstance().setPrefRateDontShowAgain(true);

        assertFalse(
                RateUtils.getInstance().shouldShowRateDialog(ContextUtils.getApplicationContext()));
    }
}
