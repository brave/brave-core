
/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab;

import android.content.SharedPreferences;
import java.util.Calendar;
import android.os.Build;

import androidx.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabLaunchType;
import org.chromium.chrome.browser.preferences.BackgroundImagesPreferences;
import org.chromium.chrome.browser.ntp.sponsored.BackgroundImage;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImage;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;

public class BraveTab extends Tab {
	private static final String TAG = "BraveTab";

	private BackgroundImage backgroundImage;
    private int index;

	BraveTab(int id, Tab parent, boolean incognito, @Nullable @TabLaunchType Integer launchType) {
		super(id,parent,incognito,launchType);

		if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            backgroundImage = getBackgroundImage();
        }
        index = SponsoredImageUtil.imageIndex;
	}

	public BackgroundImage getTabBackgroundImage() {
        return backgroundImage;
    }

    public int getIndex() {
        return index;
    }

    private BackgroundImage getBackgroundImage() {
        BackgroundImage backgroundImage;
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();

        if (mSharedPreferences.getInt(BackgroundImagesPreferences.PREF_APP_OPEN_COUNT, 0) == 2
            && SponsoredImageUtil.imageIndex == 2) {
            SponsoredImage sponsoredImage = SponsoredImageUtil.getSponsoredImage(); 
            long currentTime = Calendar.getInstance().getTimeInMillis();
            if ((sponsoredImage.getStartDate() <= currentTime  && currentTime <= sponsoredImage.getEndDate()) 
                && mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_SPONSORED_IMAGES, true)) {
                SponsoredImageUtil.imageIndex = SponsoredImageUtil.imageIndex + 3;
                return sponsoredImage;
            }
        }

        if (SponsoredImageUtil.imageIndex % 4 == 0 && SponsoredImageUtil.imageIndex != 1) {
            SponsoredImage sponsoredImage = SponsoredImageUtil.getSponsoredImage(); 
            long currentTime = Calendar.getInstance().getTimeInMillis();
            if ((sponsoredImage.getStartDate() <= currentTime  && currentTime <= sponsoredImage.getEndDate()) 
                && mSharedPreferences.getInt(BackgroundImagesPreferences.PREF_APP_OPEN_COUNT, 0) != 1
                && mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_SPONSORED_IMAGES, true)) {
                backgroundImage = sponsoredImage;
            } else {
                backgroundImage = SponsoredImageUtil.getBackgroundImage();
            }
        } else {
            backgroundImage = SponsoredImageUtil.getBackgroundImage();
        }

        SponsoredImageUtil.imageIndex++;

        return backgroundImage;
    }
}