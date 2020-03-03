/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.tile;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;
import android.content.SharedPreferences;

import org.chromium.chrome.R;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp.sponsored.NTPUtil;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;

public class BraveTileWithTextView extends TileWithTextView {
	public BraveTileWithTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setTitle(String title, int titleLines) {

    	super.setTitle(title, titleLines);

        TextView mTitleView = findViewById(R.id.tile_view_title);

    	SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        boolean isMoreTabs = false;
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            isMoreTabs = tabModel.getCount() >= SponsoredImageUtil.MAX_TABS ? true : false;
        }

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)
            && NTPUtil.shouldEnableNTPFeature(isMoreTabs)) {
            mTitleView.setTextColor(getResources().getColor(android.R.color.white));
        }
    }
}