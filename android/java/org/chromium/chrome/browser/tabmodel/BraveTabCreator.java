/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import android.app.Activity;
import android.os.Build;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.init.StartupTabPreloader;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.WindowAndroid;

public class BraveTabCreator extends ChromeTabCreator {
    public BraveTabCreator(Activity activity, WindowAndroid nativeWindow,
            StartupTabPreloader startupTabPreloader,
            Supplier<TabDelegateFactory> tabDelegateFactory, boolean incognito,
            OverviewNTPCreator overviewNTPCreator, AsyncTabParamsManager asyncTabParamsManager,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            ObservableSupplier<CompositorViewHolder> compositorViewHolderSupplier) {
        super(activity, nativeWindow, startupTabPreloader, tabDelegateFactory, incognito,
                overviewNTPCreator, asyncTabParamsManager, tabModelSelectorSupplier,
                compositorViewHolderSupplier);
    }

    @Override
    public Tab launchUrl(String url, @TabLaunchType int type) {
        if (url.equals(UrlConstants.NTP_URL) && type == TabLaunchType.FROM_CHROME_UI) {
            registerPageView();
            ChromeTabbedActivity chromeTabbedActivity = BraveActivity.getChromeTabbedActivity();
            if(chromeTabbedActivity != null && Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
                TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
                if (tabModel.getCount() >= SponsoredImageUtil.MAX_TABS && UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                    Tab tab = (Tab) BraveReflectionUtil.InvokeMethod(BraveActivity.class,
                            chromeTabbedActivity, "selectExistingTab", String.class,
                            UrlConstants.NTP_URL);
                    if (tab != null) {
                        BraveReflectionUtil.InvokeMethod(
                                ChromeTabbedActivity.class, chromeTabbedActivity, "hideOverview");
                        return tab;
                    }
                }
            }
        }
        return super.launchUrl(url, type);
    }

    private void registerPageView() {
        NTPBackgroundImagesBridge.getInstance(Profile.getLastUsedRegularProfile()).registerPageView();
    }
}
