/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertSame;

import android.os.Bundle;
import android.widget.FrameLayout;

import androidx.recyclerview.widget.RecyclerView;
import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragment;
import org.chromium.components.browser_ui.site_settings.AllSiteSettings;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;

/** Tests which view receives insets on Settings pages with footer buttons. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveSettingsActivityTest {
    private FrameLayout mRoot;
    private RecyclerView mList;

    @Before
    public void setUp() {
        mRoot = new FrameLayout(ApplicationProvider.getApplicationContext());
        mList = new RecyclerView(mRoot.getContext());
        mList.setId(R.id.recycler_view);
        mRoot.addView(mList);
    }

    @Test
    public void testClearBrowsingDataInsetsIncludeFooter() {
        assertSame(
                mRoot,
                BraveSettingsActivity.getInsetView(new BraveClearBrowsingDataFragment(), mRoot));
    }

    @Test
    public void testStorageInsetsIncludeFooter() {
        assertSame(
                mRoot,
                BraveSettingsActivity.getInsetView(
                        createSiteSettings(SiteSettingsCategory.Type.USE_STORAGE), mRoot));
    }

    @Test
    public void testZoomInsetsIncludeFooter() {
        assertSame(
                mRoot,
                BraveSettingsActivity.getInsetView(
                        createSiteSettings(SiteSettingsCategory.Type.ZOOM), mRoot));
    }

    @Test
    public void testAllSitesInsetsOnlyTheList() {
        assertSame(
                mList,
                BraveSettingsActivity.getInsetView(
                        createSiteSettings(SiteSettingsCategory.Type.ALL_SITES), mRoot));
    }

    private static AllSiteSettings createSiteSettings(@SiteSettingsCategory.Type int category) {
        AllSiteSettings fragment = new AllSiteSettings();
        Bundle args = new Bundle();
        args.putString(
                AllSiteSettings.EXTRA_CATEGORY, SiteSettingsCategory.preferenceKey(category));
        fragment.setArguments(args);
        return fragment;
    }
}
