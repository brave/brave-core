/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import android.app.Activity;
import android.os.Build;

import androidx.annotation.Nullable;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.new_tab_url.DseNewTabUrlManager;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.WindowAndroid;

public class BraveTabCreator extends ChromeTabCreator {
    public BraveTabCreator(
            Activity activity,
            WindowAndroid nativeWindow,
            Supplier<TabDelegateFactory> tabDelegateFactory,
            OneshotSupplier<ProfileProvider> profileProviderSupplier,
            boolean incognito,
            AsyncTabParamsManager asyncTabParamsManager,
            Supplier<TabModelSelector> tabModelSelectorSupplier,
            Supplier<CompositorViewHolder> compositorViewHolderSupplier,
            @Nullable DseNewTabUrlManager dseNewTabUrlManager) {
        super(
                activity,
                nativeWindow,
                tabDelegateFactory,
                profileProviderSupplier,
                incognito,
                asyncTabParamsManager,
                tabModelSelectorSupplier,
                compositorViewHolderSupplier,
                dseNewTabUrlManager);
    }

    @Override
    public Tab launchUrl(String url, @TabLaunchType int type) {
        if (url.equals(UrlConstants.NTP_URL)
                && (type == TabLaunchType.FROM_CHROME_UI || type == TabLaunchType.FROM_STARTUP
                        || type == TabLaunchType.FROM_TAB_SWITCHER_UI)) {
            registerPageView();
            ChromeTabbedActivity chromeTabbedActivity = BraveActivity.getChromeTabbedActivity();
            if (chromeTabbedActivity != null && Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
                TabModel tabModel = chromeTabbedActivity.getCurrentTabModel();
                if (tabModel.getCount() >= SponsoredImageUtil.MAX_TABS
                        && UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                                .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                    Tab tab =
                            BraveActivity.class
                                    .cast(chromeTabbedActivity)
                                    .selectExistingTab(UrlConstants.NTP_URL);
                    if (tab != null) {
                        BraveReflectionUtil.invokeMethod(
                                ChromeTabbedActivity.class, chromeTabbedActivity, "hideOverview");
                        return tab;
                    }
                }
            }
        }
        return super.launchUrl(url, type);
    }

    @Override
    public Tab createNewTab(LoadUrlParams loadUrlParams, @TabLaunchType int type, Tab parent) {
        if (loadUrlParams.getUrl().equals(UrlConstants.NTP_URL)
                && type == TabLaunchType.FROM_TAB_GROUP_UI) {
            registerPageView();
        }
        return super.createNewTab(loadUrlParams, type, parent, null);
    }

    private void registerPageView() {
        NTPBackgroundImagesBridge.getInstance(ProfileManager.getLastUsedRegularProfile())
                .registerPageView();
    }
}
