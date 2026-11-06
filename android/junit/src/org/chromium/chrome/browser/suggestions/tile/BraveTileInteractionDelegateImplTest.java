/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.view.View;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preloading.AndroidPrerenderManager;
import org.chromium.components.browser_ui.test.BrowserUiTestFragmentActivity;

/**
 * Unit tests for {@link BraveTileInteractionDelegateImpl}. Verifies that the "Add site" long-press
 * menu item is only offered in shortcuts (pinned) mode, since adding a custom shortcut only makes
 * sense for the pinned-tiles list, not for frequency-based suggestions.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveTileInteractionDelegateImplTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private ContextMenuManager mContextMenuManager;
    @Mock private TileGroup.Delegate mTileGroupDelegate;
    @Mock private TileDragDelegate mTileDragDelegate;
    @Mock private TileGroup.CustomTileModificationDelegate mCustomTileModificationDelegate;
    @Mock private Tile mTile;
    @Mock private AndroidPrerenderManager mAndroidPrerenderManager;

    private BraveTileInteractionDelegateImpl mDelegate;

    @Before
    public void setUp() {
        // Avoids a real JNI call inside AndroidPrerenderManager.getAndroidPrerenderManager(),
        // which TileInteractionDelegateImpl's constructor invokes.
        AndroidPrerenderManager.setAndroidPrerenderManagerForTesting(mAndroidPrerenderManager);

        Activity activity =
                Robolectric.buildActivity(BrowserUiTestFragmentActivity.class).setup().get();

        mDelegate =
                new BraveTileInteractionDelegateImpl(
                        mContextMenuManager,
                        mTileGroupDelegate,
                        mTileDragDelegate,
                        mCustomTileModificationDelegate,
                        mTile,
                        new View(activity));
    }

    @After
    public void tearDown() {
        AndroidPrerenderManager.clearAndroidPrerenderManagerForTesting();
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE);
    }

    @Test
    public void shortcutsMode_addSiteSupported() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);

        assertTrue(mDelegate.isBraveItemSupported(BraveTileInteractionDelegateImpl.BRAVE_ADD_SITE));
    }

    @Test
    public void frequentMode_addSiteNotSupported() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        assertFalse(
                mDelegate.isBraveItemSupported(BraveTileInteractionDelegateImpl.BRAVE_ADD_SITE));
    }

    @Test
    public void modeSwitchAndHideWidgetItems_alwaysSupportedRegardlessOfMode() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        assertTrue(
                mDelegate.isBraveItemSupported(
                        BraveTileInteractionDelegateImpl.BRAVE_SHOW_FREQUENT));
        assertTrue(
                mDelegate.isBraveItemSupported(
                        BraveTileInteractionDelegateImpl.BRAVE_SHOW_SHORTCUTS));
        assertTrue(
                mDelegate.isBraveItemSupported(BraveTileInteractionDelegateImpl.BRAVE_HIDE_WIDGET));
    }

    @Test
    public void unknownMenuItem_notSupported() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);

        assertFalse(mDelegate.isBraveItemSupported(-1));
    }
}
