/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.native_page.BraveNtpDelegate;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/**
 * Unit tests for the NTP top-sites display-mode preference ({@link NtpUtil#getTopSitesDisplayMode}
 * / {@link NtpUtil#setTopSitesDisplayMode}) and the {@link BraveNtpDelegate} menu-item ID
 * constants.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveNtpTopSitesModeUnitTest {

    @After
    public void tearDown() {
        // Reset to default so tests don't bleed into each other.
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE);
    }

    @Test
    public void testDefaultModeIsShortcuts() {
        assertEquals(NtpUtil.TOP_SITES_MODE_SHORTCUTS, NtpUtil.getTopSitesDisplayMode());
    }

    @Test
    public void testSetFrequentModeAndReadBack() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        assertEquals(NtpUtil.TOP_SITES_MODE_FREQUENT, NtpUtil.getTopSitesDisplayMode());
    }

    @Test
    public void testSetShortcutsModeAndReadBack() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        assertEquals(NtpUtil.TOP_SITES_MODE_SHORTCUTS, NtpUtil.getTopSitesDisplayMode());
    }

    @Test
    public void testModesAreDistinct() {
        assertNotEquals(NtpUtil.TOP_SITES_MODE_SHORTCUTS, NtpUtil.TOP_SITES_MODE_FREQUENT);
    }

    /**
     * Brave item IDs must be >= ContextMenuItemId.NUM_ENTRIES so they never shadow a current or
     * future Chromium value in a switch statement.
     */
    @Test
    public void testBraveItemIds_doNotCollideWithChromiumRange() {
        // ContextMenuItemId.NUM_ENTRIES = 18 — hardcoded here so this test
        // catches if the Brave IDs are ever accidentally lowered.
        final int chromiumNumEntries = 18;
        assertTrue(BraveNtpDelegate.BRAVE_ADD_SITE >= chromiumNumEntries);
        assertTrue(BraveNtpDelegate.BRAVE_SHOW_FREQUENT >= chromiumNumEntries);
        assertTrue(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS >= chromiumNumEntries);
        assertTrue(BraveNtpDelegate.BRAVE_HIDE_WIDGET >= chromiumNumEntries);
    }

    @Test
    public void testBraveItemIds_areAllUnique() {
        int[] ids = {
            BraveNtpDelegate.BRAVE_ADD_SITE,
            BraveNtpDelegate.BRAVE_SHOW_FREQUENT,
            BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS,
            BraveNtpDelegate.BRAVE_HIDE_WIDGET
        };
        for (int i = 0; i < ids.length; i++) {
            for (int j = i + 1; j < ids.length; j++) {
                assertFalse(
                        "IDs at index " + i + " and " + j + " must be unique", ids[i] == ids[j]);
            }
        }
    }

    @Test
    public void testShortcutsModeIsZero() {
        assertEquals(0, NtpUtil.TOP_SITES_MODE_SHORTCUTS);
    }

    @Test
    public void testFrequentModeIsOne() {
        assertEquals(1, NtpUtil.TOP_SITES_MODE_FREQUENT);
    }
}
