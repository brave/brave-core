/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.build.NullUtil.assertNonNull;

import android.app.Activity;
import android.view.LayoutInflater;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.EnsuresNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.feed.BraveFeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.feed.FeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedSurfaceProvider;
import org.chromium.chrome.browser.feed.FeedSwipeRefreshLayout;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.magic_stack.ModuleRegistry;
import org.chromium.chrome.browser.metrics.StartupMetricsTracker;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.ntp_customization.edge_to_edge.TopInsetCoordinator;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.HomeSurfaceTracker;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.WindowAndroid;

import java.util.function.Supplier;

@NullMarked
public class BraveNewTabPage extends NewTabPage {
    // To delete in bytecode, members from parent class will be used instead.
    private @Nullable BrowserControlsStateProvider mBrowserControlsStateProvider;
    private @Nullable NewTabPageLayout mNewTabPageLayout;

    @SuppressWarnings("UnusedVariable")
    private @Nullable FeedSurfaceProvider mFeedSurfaceProvider;

    private @Nullable Supplier<Toolbar> mToolbarSupplier;
    private @Nullable BottomSheetController mBottomSheetController;
    private @Nullable ObservableSupplier<Integer> mTabStripHeightSupplier;

    public BraveNewTabPage(
            Activity activity,
            BrowserControlsStateProvider browserControlsStateProvider,
            Supplier<@Nullable Tab> activityTabProvider,
            SnackbarManager snackbarManager,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            TabModelSelector tabModelSelector,
            boolean isTablet,
            NewTabPageCreationTracker tabCreationTracker,
            boolean isInNightMode,
            NativePageHost nativePageHost,
            Tab tab,
            String url,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier,
            WindowAndroid windowAndroid,
            Supplier<Toolbar> toolbarSupplier,
            @Nullable HomeSurfaceTracker homeSurfaceTracker,
            ObservableSupplier<TabContentManager> tabContentManagerSupplier,
            ObservableSupplier<Integer> tabStripHeightSupplier,
            OneshotSupplier<ModuleRegistry> moduleRegistrySupplier,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            ObservableSupplier<TopInsetCoordinator> topInsetCoordinatorSupplier,
            StartupMetricsTracker startupMetricsTracker,
            MultiInstanceManager multiInstanceManager) {
        super(
                activity,
                browserControlsStateProvider,
                activityTabProvider,
                snackbarManager,
                lifecycleDispatcher,
                tabModelSelector,
                isTablet,
                tabCreationTracker,
                isInNightMode,
                nativePageHost,
                tab,
                url,
                bottomSheetController,
                shareDelegateSupplier,
                windowAndroid,
                toolbarSupplier,
                homeSurfaceTracker,
                tabContentManagerSupplier,
                tabStripHeightSupplier,
                moduleRegistrySupplier,
                edgeToEdgeControllerSupplier,
                topInsetCoordinatorSupplier,
                startupMetricsTracker,
                multiInstanceManager);

        assertNonNull(mNewTabPageLayout);
        assert mNewTabPageLayout instanceof BraveNewTabPageLayout;
        if (mNewTabPageLayout instanceof BraveNewTabPageLayout) {
            ((BraveNewTabPageLayout) mNewTabPageLayout).setTab(tab);
            ((BraveNewTabPageLayout) mNewTabPageLayout).setTabProvider(activityTabProvider);
        }

        // We have no way to know exactly which service the observer is added to, so try
        // remove on
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
        TemplateUrlService templateUrlService =
                TemplateUrlServiceFactory.getForProfile(
                        Profile.fromWebContents(assertNonNull(mTab.getWebContents())));
        templateUrlService.addObserver(this);
    }

    @Override
    @EnsuresNonNull({"mNewTabPageLayout", "mFeedSurfaceProvider"})
    protected void initializeMainView(
            Activity activity,
            WindowAndroid windowAndroid,
            SnackbarManager snackbarManager,
            boolean isInNightMode,
            Supplier<ShareDelegate> shareDelegateSupplier,
            String url,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            StartupMetricsTracker startupMetricsTracker) {
        // Override surface provider
        Profile profile = Profile.fromWebContents(mTab.getWebContents());
        assertNonNull(profile);

        LayoutInflater inflater = LayoutInflater.from(activity);
        mNewTabPageLayout = (NewTabPageLayout) inflater.inflate(R.layout.new_tab_page_layout, null);

        assertNonNull(mBrowserControlsStateProvider);
        assertNonNull(mToolbarSupplier);
        assertNonNull(mTabStripHeightSupplier);
        assert !FeedFeatures.isFeedEnabled(profile);
        FeedSurfaceCoordinator feedSurfaceCoordinator =
                new BraveFeedSurfaceCoordinator(
                        activity,
                        snackbarManager,
                        windowAndroid,
                        new SnapScrollHelperImpl(mNewTabPageManager, mNewTabPageLayout),
                        mNewTabPageLayout,
                        mBrowserControlsStateProvider.getTopControlsHeight(),
                        isInNightMode,
                        this,
                        profile,
                        mBottomSheetController,
                        shareDelegateSupplier,
                        /* externalScrollableContainerDelegate= */ null,
                        NewTabPageUtils.decodeOriginFromNtpUrl(url),
                        PrivacyPreferencesManagerImpl.getInstance(),
                        mToolbarSupplier,
                        mConstructedTimeNs,
                        FeedSwipeRefreshLayout.create(activity, R.id.toolbar_container),
                        /* overScrollDisabled= */ false,
                        /* viewportView= */ null,
                        /* actionDelegate= */ null,
                        mTabStripHeightSupplier,
                        edgeToEdgeControllerSupplier);

        mFeedSurfaceProvider = feedSurfaceCoordinator;
    }

    public boolean updateSearchProvider() {
        // Search provider logo is not used in Brave's NTP.
        mSearchProviderHasLogo = false;
        return false;
    }
}
