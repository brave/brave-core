/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Intent;
import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.IntentUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.ChromeInactivityTracker;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Unit tests for {@link BraveReturnToChromeUtil} class. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveReturnToChromeUtilUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock private ChromeInactivityTracker mInactivityTracker;

    @Test
    @SmallTest
    public void testShouldNotShowNtpWithExternalViewIntent() {
        // Simulate an ACTION_VIEW intent from an external app (e.g., clicking a link in Gmail).
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://example.com"));

        assertFalse(IntentUtils.isMainIntentFromLauncher(intent));
        assertFalse(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        intent, null, mInactivityTracker));

        // Verify snackbar flag was NOT set.
        assertFalse(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false));
    }

    @Test
    @SmallTest
    public void testShouldNotShowNtpWithSendIntent() {
        // Simulate an ACTION_SEND intent (e.g., sharing a link to Brave).
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.putExtra(Intent.EXTRA_TEXT, "https://example.com");

        assertFalse(IntentUtils.isMainIntentFromLauncher(intent));
        assertFalse(
                BraveReturnToChromeUtil.shouldShowNtpAsHomeSurfaceAtStartup(
                        intent, null, mInactivityTracker));
    }

    @Test
    @SmallTest
    public void testMainLauncherIntentIsRecognized() {
        // Verify that a proper main launcher intent passes the intent check.
        Intent intent = createMainIntentFromLauncher();
        assertTrue(IntentUtils.isMainIntentFromLauncher(intent));
    }

    private Intent createMainIntentFromLauncher() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        return intent;
    }
}
