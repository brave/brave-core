/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.google.common.collect.ImmutableSet;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/**
 * Unit tests for {@link BraveDefaultPaneOrderController}. Verifies that the Tab Groups pane is
 * dropped from the Hub pane order when the "Enable tab groups" master switch is off, and kept
 * otherwise (matching the upstream order).
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveDefaultPaneOrderControllerUnitTest {
    private final BraveDefaultPaneOrderController mController =
            new BraveDefaultPaneOrderController();

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testTabGroupsPaneKeptByDefault() {
        // No pref written: the switch defaults to on, so the pane order is untouched.
        assertEquals(new DefaultPaneOrderController().getPaneOrder(), mController.getPaneOrder());
        assertTrue(mController.getPaneOrder().contains(PaneId.TAB_GROUPS));
    }

    @Test
    public void testTabGroupsPaneKeptWhenEnabled() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true);

        assertEquals(new DefaultPaneOrderController().getPaneOrder(), mController.getPaneOrder());
    }

    @Test
    public void testTabGroupsPaneRemovedWhenDisabled() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);

        ImmutableSet<Integer> paneOrder = mController.getPaneOrder();
        ImmutableSet<Integer> upstreamOrder = new DefaultPaneOrderController().getPaneOrder();

        // Only the Tab Groups pane is dropped; every other pane is preserved in order.
        assertFalse(paneOrder.contains(PaneId.TAB_GROUPS));
        assertEquals(upstreamOrder.size() - 1, paneOrder.size());
        for (Integer paneId : upstreamOrder) {
            if (paneId != PaneId.TAB_GROUPS) {
                assertTrue(paneOrder.contains(paneId));
            }
        }
    }
}
