/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.content.SharedPreferences;

import org.chromium.chrome.R;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;
import org.chromium.chrome.browser.suggestions.tile.SiteSection;
import org.chromium.chrome.browser.explore_sites.ExploreSitesBridge;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp.sponsored.NTPUtil;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;

public class BraveNewTabPageLayout extends NewTabPageLayout {
    private ViewGroup mBraveStatsView;

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setSearchProviderInfo(boolean hasLogo, boolean isGoogle) {
        super.setSearchProviderInfo(hasLogo, isGoogle);
        // Make brave stats visibile always on NTP.
        // NewTabPageLayout::setSearchProviderInfo() makes it invisible.
        // So, explicitly set it as visible.
        mBraveStatsView.setVisibility(View.VISIBLE);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mBraveStatsView = (ViewGroup) findViewById(R.id.brave_stats);
    }

    @Override
    public void insertSiteSectionView() {
        ViewGroup mainLayout = findViewById(R.id.ntp_main_layout);
        mSiteSectionView = SiteSection.inflateSiteSection(mainLayout);
        ViewGroup.LayoutParams layoutParams = mSiteSectionView.getLayoutParams();
        layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        // If the explore sites section exists as its own section, then space it more closely.
        int variation = ExploreSitesBridge.getVariation();
        if (ExploreSitesBridge.isEnabled(variation)
                && !ExploreSitesBridge.isIntegratedWithMostLikely(variation)) {
            ((MarginLayoutParams) layoutParams).bottomMargin =
                    getResources().getDimensionPixelOffset(
                            R.dimen.tile_grid_layout_vertical_spacing);
        }
        mSiteSectionView.setLayoutParams(layoutParams);

        ViewGroup mBraveStatsView = (ViewGroup) findViewById(R.id.brave_stats_layout);
        int insertionPoint = mainLayout.indexOfChild(mBraveStatsView) + 1;
        mainLayout.addView(mSiteSectionView, insertionPoint);
    }

    @Override
    public int getMaxTileRows() {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        boolean isMoreTabs = false;
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            isMoreTabs = tabModel.getCount() >= SponsoredImageUtil.MAX_TABS ? true : false;
        }

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true) 
            && NTPUtil.shouldEnableNTPFeature(isMoreTabs)) {
            return 1;
        } else {
            return 2;
        }
    }
}
