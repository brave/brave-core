/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import static org.junit.Assert.assertEquals;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.browser_ui.test.BrowserUiTestFragmentActivity;
import org.chromium.components.browser_ui.widget.R;

/**
 * Unit tests for {@link BraveTileView}. Verifies the pinned-shortcut badge is suppressed in "Show
 * shortcuts" mode, where every tile is a shortcut and the badge would be redundant.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveTileViewTest {
    private Activity mActivity;
    private BraveTileView mTileView;
    private ImageView mPinnedShortcutBadgeView;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(BrowserUiTestFragmentActivity.class).setup().get();

        mTileView = new BraveTileView(mActivity, null);
        LayoutInflater.from(mActivity).inflate(R.layout.tile_view_modern, mTileView, true);
        // Needed because tile_view_modern is a <merge> layout, which LayoutInflater does not
        // call onFinishInflate() for automatically.
        mTileView.onFinishInflate();

        mPinnedShortcutBadgeView = mTileView.findViewById(R.id.pinned_shortcut_badge);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE);
    }

    @Test
    public void frequentMode_requestedVisible_badgeShown() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        mTileView.togglePinnedShortcutBadge(true);

        assertEquals(View.VISIBLE, mPinnedShortcutBadgeView.getVisibility());
    }

    @Test
    public void frequentMode_requestedHidden_badgeHidden() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        mTileView.togglePinnedShortcutBadge(false);

        assertEquals(View.GONE, mPinnedShortcutBadgeView.getVisibility());
    }

    @Test
    public void shortcutsMode_requestedVisible_badgeSuppressed() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);

        mTileView.togglePinnedShortcutBadge(true);

        assertEquals(View.GONE, mPinnedShortcutBadgeView.getVisibility());
    }

    @Test
    public void shortcutsMode_requestedHidden_badgeStillHidden() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);

        mTileView.togglePinnedShortcutBadge(false);

        assertEquals(View.GONE, mPinnedShortcutBadgeView.getVisibility());
    }
}
