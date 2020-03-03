/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.sponsored;

import android.os.Build;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.ntp.sponsored.NTPImage;
import org.chromium.chrome.browser.ntp.sponsored.NTPUtil;
import org.chromium.chrome.browser.ntp.sponsored.NewTabListener;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.BraveRewardsHelper;

public class SponsoredTab{
    private NTPImage ntpImage;
    private int tabIndex;
    private boolean mShouldShowBanner;
    private boolean isMoreTabs;
    private NewTabListener newTabListener;

    public SponsoredTab() {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            isMoreTabs = tabModel.getCount() > SponsoredImageUtil.MAX_TABS ? true : false;
        }

        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M || (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP && !isMoreTabs)){
            ntpImage = NTPUtil.getNTPImage();
            tabIndex = SponsoredImageUtil.getTabIndex();
            updateBannerPref();
        }
    }

    public NTPImage getTabNTPImage() {
        if (ntpImage != null) {
            return ntpImage;
        } else {
            return NTPUtil.getNTPImage();
        }
    }

    public void setNTPImage(NTPImage ntpImage) {
        this.ntpImage = ntpImage;
    }

    public void setNewTabListener(NewTabListener newTabListener) {
        this.newTabListener = newTabListener;
    }

    public int getTabIndex() {
        return tabIndex;
    }

    public void setTanIndex(int tabIndex) {
        this.tabIndex = tabIndex;
    }

    public boolean shouldShowBanner() {
        return mShouldShowBanner;
    }

    public void updateBannerPref() {
        mShouldShowBanner = ContextUtils.getAppSharedPreferences().getBoolean(BackgroundImagesPreferences.PREF_SHOW_NON_DISTRUPTIVE_BANNER, true);
    }

    public boolean isMoreTabs() {
        return isMoreTabs;
    }

    public void setMoreTabs(boolean isMoreTabs) {
        this.isMoreTabs = isMoreTabs;
    }
}