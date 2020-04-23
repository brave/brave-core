/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;
import org.chromium.chrome.browser.suggestions.tile.SiteSection;
import org.chromium.chrome.browser.explore_sites.ExploreSitesBridge;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;

public class BraveNewTabPageLayout extends NewTabPageLayout {
    private ViewGroup mBraveStatsView;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        Profile mProfile = Profile.getLastUsedProfile();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
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
    protected void insertSiteSectionView() {
        ViewGroup mainLayout = findViewById(R.id.ntp_main_layout);

        mSiteSectionView = SiteSection.inflateSiteSection(mainLayout);
        ViewGroup.LayoutParams layoutParams = mSiteSectionView.getLayoutParams();
        layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        // If the explore sites section exists as its own section, then space it more closely.
        int variation = ExploreSitesBridge.getVariation();
        if (ExploreSitesBridge.isEnabled(variation)) {
            ((MarginLayoutParams) layoutParams).bottomMargin =
                    getResources().getDimensionPixelOffset(
                            R.dimen.tile_grid_layout_vertical_spacing);
        }
        mSiteSectionView.setLayoutParams(layoutParams);

        ViewGroup mBraveStatsView = (ViewGroup) findViewById(R.id.brave_stats_layout);
        int insertionPoint = mainLayout.indexOfChild(mBraveStatsView) + 1;
        if (!mNTPBackgroundImagesBridge.isSuperReferral()
            || !NTPBackgroundImagesBridge.enableSponsoredImages())
            mainLayout.addView(mSiteSectionView, insertionPoint);
    }

    @Override
    protected int getMaxTileRows() {
        boolean isMoreTabs = false;
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            isMoreTabs = tabModel.getCount() >= SponsoredImageUtil.MAX_TABS ? true : false;
        }

        if(BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)
            && NTPUtil.shouldEnableNTPFeature(isMoreTabs)) {
            return 1;
        } else {
            return 2;
        }
    }
}
