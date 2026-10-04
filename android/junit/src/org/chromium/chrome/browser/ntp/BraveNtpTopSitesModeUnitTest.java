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
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.AppearancePreferences;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;

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
    public void testDefaultModeIsFrequent() {
        assertEquals(NtpUtil.TOP_SITES_MODE_FREQUENT, NtpUtil.getTopSitesDisplayMode());
    }

    @Test
    public void testSetFrequentModeAndReadBack() {
        // FREQUENT is now the default, so set SHORTCUTS first to prove the write path is
        // actually exercised rather than vacuously matching the fallback default.
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        assertEquals(NtpUtil.TOP_SITES_MODE_FREQUENT, NtpUtil.getTopSitesDisplayMode());
    }

    @Test
    public void testSetShortcutsModeAndReadBack() {
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
        int chromiumNumEntries = ContextMenuManager.ContextMenuItemId.NUM_ENTRIES;
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

    /**
     * NtpUtil duplicates these pref key string literals rather than importing the settings package
     * (see the comment above them), so nothing at compile time catches the two copies drifting
     * apart. Writing through the settings-side constant and reading through NtpUtil (or vice versa)
     * only agrees if the literals are still identical.
     */
    @Test
    public void testShowTopSitesPrefKeyMatchesSettings() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BackgroundImagesPreferences.PREF_SHOW_TOP_SITES, false);
        try {
            assertFalse(NtpUtil.shouldDisplayTopSites());
        } finally {
            ChromeSharedPreferences.getInstance()
                    .removeKey(BackgroundImagesPreferences.PREF_SHOW_TOP_SITES);
        }
    }

    @Test
    public void testShowBraveStatsPrefKeyMatchesSettings() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS, false);
        try {
            assertFalse(NtpUtil.shouldDisplayBraveStats());
        } finally {
            ChromeSharedPreferences.getInstance()
                    .removeKey(BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS);
        }
    }

    @Test
    public void testShowBraveRewardsIconPrefKeyMatchesSettings() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(AppearancePreferences.PREF_SHOW_BRAVE_REWARDS_ICON, false);
        try {
            assertFalse(NtpUtil.shouldShowRewardsIcon());
        } finally {
            ChromeSharedPreferences.getInstance()
                    .removeKey(AppearancePreferences.PREF_SHOW_BRAVE_REWARDS_ICON);
        }
    }
}
