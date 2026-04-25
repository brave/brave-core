/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quickactionsearchandbookmark;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Intent;

import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.IntentUtils;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.BraveIntentHandler;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Tests for the QuickActionSearchAndBookmarkWidgetProvider. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class QuickActionSearchAndBookmarkWidgetProviderTest {

    private static final class TestContext extends AdvancedMockContext {
        public TestContext() {
            super(
                    InstrumentationRegistry.getInstrumentation()
                            .getTargetContext()
                            .getApplicationContext());
        }
    }

    private QuickActionSearchAndBookmarkWidgetProviderTest.TestContext mContext;

    @Before
    public void setUp() {
        mContext = new QuickActionSearchAndBookmarkWidgetProviderTest.TestContext();
    }

    @Test
    @SmallTest
    public void testLeoIntent() {
        Intent intent = QuickActionSearchAndBookmarkWidgetProvider.createLeoIntent(mContext);
        assertTrue(
                IntentUtils.safeGetBooleanExtra(
                        intent, IntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET, false));
        assertTrue(
                IntentUtils.safeGetBooleanExtra(
                        intent, BraveIntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET_LEO, false));
        assertFalse(
                IntentUtils.safeGetBooleanExtra(
                        intent, BraveIntentHandler.EXTRA_LEO_VOICE_PROMPT_INVOKED, false));
    }
}
