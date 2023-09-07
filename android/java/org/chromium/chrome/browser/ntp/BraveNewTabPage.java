/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.view.LayoutInflater;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.feed.BraveFeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.feed.FeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedSurfaceProvider;
import org.chromium.chrome.browser.feed.FeedSwipeRefreshLayout;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncherImpl;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.HomeSurfaceTracker;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.xsurface.feed.FeedLaunchReliabilityLogger.SurfaceType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;

public class BraveNewTabPage extends NewTabPage {
    // To delete in bytecode, members from parent class will be used instead.
    private BrowserControlsStateProvider mBrowserControlsStateProvider;
    private NewTabPageLayout mNewTabPageLayout;
    private FeedSurfaceProvider mFeedSurfaceProvider;
    private Supplier<Toolbar> mToolbarSupplier;
    private TabModelSelector mTabModelSelector;
    private BottomSheetController mBottomSheetController;

    public BraveNewTabPage(Activity activity,
            BrowserControlsStateProvider browserControlsStateProvider,
            Supplier<Tab> activityTabProvider, SnackbarManager snackbarManager,
            ActivityLifecycleDispatcher lifecycleDispatcher, TabModelSelector tabModelSelector,
            boolean isTablet, NewTabPageUma uma, boolean isInNightMode,
            NativePageHost nativePageHost, Tab tab, String url,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier, WindowAndroid windowAndroid,
            Supplier<Toolbar> toolbarSupplier, SettingsLauncher settingsLauncher,
            HomeSurfaceTracker homeSurfaceTracker,
            ObservableSupplier<TabContentManager> tabContentManagerSupplier) {
        super(activity, browserControlsStateProvider, activityTabProvider, snackbarManager,
                lifecycleDispatcher, tabModelSelector, isTablet, uma, isInNightMode, nativePageHost,
                tab, url, bottomSheetController, shareDelegateSupplier, windowAndroid,
                toolbarSupplier, settingsLauncher, homeSurfaceTracker, tabContentManagerSupplier);

        assert mNewTabPageLayout instanceof BraveNewTabPageLayout;
        if (mNewTabPageLayout instanceof BraveNewTabPageLayout) {
            ((BraveNewTabPageLayout) mNewTabPageLayout).setTab(tab);
            ((BraveNewTabPageLayout) mNewTabPageLayout).setTabProvider(activityTabProvider);
        }

        // We have no way to know exactly which service the observer is added to, so try remove on
        // both
        if (tabModelSelector != null) {
            for (TabModel tabModel : tabModelSelector.getModels()) {
                if (tabModel.getProfile() != null) {
                    TemplateUrlService templateUrlService =
                            TemplateUrlServiceFactory.getForProfile(tabModel.getProfile());
                    templateUrlService.removeObserver(this);
                }
            }
        }
        // Re-add to the new tab's profile
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(
                Profile.fromWebContents(mTab.getWebContents()));
        templateUrlService.addObserver(this);
    }

    @Override
    protected void initializeMainView(Activity activity, WindowAndroid windowAndroid,
            SnackbarManager snackbarManager, NewTabPageUma uma, boolean isInNightMode,
            Supplier<ShareDelegate> shareDelegateSupplier, String url) {
        // Override surface provider
        Profile profile = Profile.fromWebContents(mTab.getWebContents());

        LayoutInflater inflater = LayoutInflater.from(activity);
        mNewTabPageLayout = (NewTabPageLayout) inflater.inflate(R.layout.new_tab_page_layout, null);

        assert !FeedFeatures.isFeedEnabled();
        FeedSurfaceCoordinator feedSurfaceCoordinator = new BraveFeedSurfaceCoordinator(activity,
                snackbarManager, windowAndroid,
                new SnapScrollHelperImpl(mNewTabPageManager, mNewTabPageLayout), mNewTabPageLayout,
                mBrowserControlsStateProvider.getTopControlsHeight(), isInNightMode, this, profile,
                /* isPlaceholderShownInitially= */ false, mBottomSheetController,
                shareDelegateSupplier, /* externalScrollableContainerDelegate= */ null,
                NewTabPageUtils.decodeOriginFromNtpUrl(url),
                PrivacyPreferencesManagerImpl.getInstance(), mToolbarSupplier,
                SurfaceType.NEW_TAB_PAGE, mConstructedTimeNs,
                FeedSwipeRefreshLayout.create(activity, R.id.toolbar_container),
                /* overScrollDisabled= */ false, /* viewportView= */ null,
                /* actionDelegate= */ null, HelpAndFeedbackLauncherImpl.getForProfile(profile),
                mTabModelSelector);

        mFeedSurfaceProvider = feedSurfaceCoordinator;
    }

    public void updateSearchProviderHasLogo() {
        // Search provider logo is not used in Brave's NTP.
        mSearchProviderHasLogo = false;
    }
}
