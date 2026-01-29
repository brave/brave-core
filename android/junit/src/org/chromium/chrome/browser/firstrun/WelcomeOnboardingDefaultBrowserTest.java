/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Tests for default browser request behavior in onboarding. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class WelcomeOnboardingDefaultBrowserTest {
    @Test
    public void testMaybeRequestDefaultBrowser_requestsWhenNotDefault() {
        WelcomeOnboardingActivity activity =
                Robolectric.buildActivity(TestWelcomeOnboardingActivity.class).create().get();
        TestDefaultBrowserDelegate delegate = new TestDefaultBrowserDelegate(false);
        activity.setDefaultBrowserDelegateForTesting(delegate);

        boolean requested = activity.maybeRequestDefaultBrowser();

        assertTrue(requested);
        assertEquals(1, delegate.mRequestCount);
    }

    @Test
    public void testMaybeRequestDefaultBrowser_skipsWhenDefault() {
        WelcomeOnboardingActivity activity =
                Robolectric.buildActivity(TestWelcomeOnboardingActivity.class).create().get();
        TestDefaultBrowserDelegate delegate = new TestDefaultBrowserDelegate(true);
        activity.setDefaultBrowserDelegateForTesting(delegate);

        boolean requested = activity.maybeRequestDefaultBrowser();

        assertFalse(requested);
        assertEquals(0, delegate.mRequestCount);
    }

    private static final class TestDefaultBrowserDelegate
            implements WelcomeOnboardingActivity.DefaultBrowserDelegate {
        private final boolean mIsDefault;
        private int mRequestCount;

        private TestDefaultBrowserDelegate(boolean isDefault) {
            mIsDefault = isDefault;
        }

        @Override
        public boolean isDefaultBrowser(WelcomeOnboardingActivity activity) {
            return mIsDefault;
        }

        @Override
        public void requestSetDefaultBrowser(WelcomeOnboardingActivity activity) {
            mRequestCount++;
        }
    }

    public static class TestWelcomeOnboardingActivity extends WelcomeOnboardingActivity {
        @SuppressWarnings("MissingSuperCall")
        @Override
        protected void applyThemeOverlays() {
            // Skip theme overlays to avoid native feature checks in Robolectric.
        }

        @Override
        protected boolean shouldApplyDynamicColors() {
            return false;
        }
    }
}
