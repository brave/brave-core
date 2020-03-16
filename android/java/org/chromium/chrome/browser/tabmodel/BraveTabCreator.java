/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import android.os.Build;

import org.chromium.base.Supplier;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.init.StartupTabPreloader;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.util.UrlConstants;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ntp_sponsored_images.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp_sponsored_images.NTPSponsoredImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;

public class BraveTabCreator extends ChromeTabCreator {

	public BraveTabCreator(ChromeActivity activity,
            WindowAndroid nativeWindow,
	        StartupTabPreloader startupTabPreloader,
	        Supplier<TabDelegateFactory> tabDelegateFactory,
            boolean incognito) {
		super(activity, nativeWindow, startupTabPreloader, tabDelegateFactory, incognito);
	}

    @Override
    public void launchNTP() {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null && Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
            TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
            if (tabModel.getCount() >= SponsoredImageUtil.MAX_TABS && BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)) {
                if(chromeTabbedActivity.getActivityTab() != null && NewTabPage.isNTPUrl(chromeTabbedActivity.getActivityTab().getUrl())) {
                    chromeTabbedActivity.hideOverview();
                } else {
                    chromeTabbedActivity.openNewOrSelectExistingTab(UrlConstants.NTP_URL);
                    chromeTabbedActivity.hideOverview();
                }
            } else {
                initializeNTPBridge();
                launchUrl(UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
            }
        } else {
            initializeNTPBridge();
            launchUrl(UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
        }
    }

    private void initializeNTPBridge() {
        NTPSponsoredImagesBridge.getInstance(Profile.getLastUsedProfile()).registerPageView();
    }
}
