/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.os.Build;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.content.SharedPreferences;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;

import org.chromium.base.ContextUtils;
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
    public int getMaxTileRows() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        if(sharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true) 
            && (Build.VERSION.SDK_INT > Build.VERSION_CODES.M || (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP))) {
            return 1;
        } else {
            return 2;
        }
    }
}
