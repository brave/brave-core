/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import static org.junit.Assert.assertFalse;

import android.app.Activity;

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
import org.chromium.chrome.browser.suggestions.SuggestionsConfig.TileStyle;
import org.chromium.components.browser_ui.test.BrowserUiTestFragmentActivity;

import java.util.Collections;

/**
 * Unit tests for {@link TileRenderer}. Verifies that the "Add new" custom-links button is only
 * offered in shortcuts (pinned) mode, since adding a custom shortcut doesn't make sense for
 * frequency-based suggestions - mirrors the same gating added to the tile long-press menu's "Add
 * site" item in {@link BraveTileInteractionDelegateImpl}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class TileRendererTest {
    private TileRenderer mTileRenderer;

    @Before
    public void setUp() {
        Activity activity =
                Robolectric.buildActivity(BrowserUiTestFragmentActivity.class).setup().get();
        mTileRenderer = new TileRenderer(activity, TileStyle.MODERN, /* titleLines= */ 1, null);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE);
    }

    @Test
    public void frequentMode_addNewButtonNotShown() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);

        assertFalse(mTileRenderer.shouldShowAddNewButton(Collections.emptyList()));
    }
}
