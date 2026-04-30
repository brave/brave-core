/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.EnsuresNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.feed.BraveFeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedActionDelegate;
import org.chromium.chrome.browser.feed.FeedActionDelegate.PageLoadObserver;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.feed.FeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedSurfaceProvider;
import org.chromium.chrome.browser.feed.FeedSwipeRefreshLayout;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.magic_stack.ModuleRegistry;
import org.chromium.chrome.browser.metrics.StartupMetricsTracker;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.chrome.browser.suggestions.tile.TileSource;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.HomeSurfaceTracker;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.edge_to_edge.TopInsetProvider;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.Supplier;

@NullMarked
public class BraveNewTabPage extends NewTabPage implements NewTabPage.MostVisitedTileClickObserver {
    // To delete in bytecode, members from parent class will be used instead.
    private @Nullable BrowserControlsStateProvider mBrowserControlsStateProvider;
    private @Nullable NewTabPageLayout mNewTabPageLayout;
    private @Nullable NewTabPageCoordinator mNewTabPageCoordinator;

    @SuppressWarnings("UnusedVariable")
    private @Nullable FeedSurfaceProvider mFeedSurfaceProvider;

    private @Nullable Supplier<Toolbar> mToolbarSupplier;
    private final BottomSheetController mBottomSheetController;
    private final NonNullObservableSupplier<Integer> mTabStripHeightSupplier;

    private final Activity mActivity;

    public BraveNewTabPage(
            Activity activity,
            BrowserControlsStateProvider browserControlsStateProvider,
            Supplier<@Nullable Tab> activityTabProvider,
            ModalDialogManager modalDialogManager,
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
            Supplier<@Nullable ShareDelegate> shareDelegateSupplier,
            WindowAndroid windowAndroid,
            Supplier<Toolbar> toolbarSupplier,
            @Nullable HomeSurfaceTracker homeSurfaceTracker,
            ActivityResultTracker activityResultTracker,
            NonNullObservableSupplier<Integer> tabStripHeightSupplier,
            OneshotSupplier<ModuleRegistry> moduleRegistrySupplier,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            TopInsetProvider topInsetProvider,
            StartupMetricsTracker startupMetricsTracker) {
        super(
                activity,
                browserControlsStateProvider,
                activityTabProvider,
                modalDialogManager,
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
                activityResultTracker,
                tabStripHeightSupplier,
                moduleRegistrySupplier,
                edgeToEdgeControllerSupplier,
                topInsetProvider,
                startupMetricsTracker);

        mActivity = activity;
        mBottomSheetController = bottomSheetController;
        mTabStripHeightSupplier = tabStripHeightSupplier;

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

        addMostVisitedTileClickObserver(this);
    }

    @Override
    public void destroy() {
        removeMostVisitedTileClickObserver(this);
        super.destroy();
    }

    @Override
    public void onMostVisitedTileClicked(Tile tile, Tab tab) {
        if (!(mActivity instanceof BraveActivity braveActivity)) return;
        MiscAndroidMetrics miscAndroidMetrics = braveActivity.getMiscAndroidMetrics();
        if (miscAndroidMetrics == null) return;
        miscAndroidMetrics.recordTopSiteNavigation(tile.getSource() == TileSource.CUSTOM_LINKS);
    }

    @Override
    @EnsuresNonNull({"mFeedSurfaceProvider"})
    protected void initializeFeedSurfaceProvider(
            Activity activity,
            WindowAndroid windowAndroid,
            ActivityResultTracker activityResultTracker,
            SnackbarManager snackbarManager,
            boolean isInNightMode,
            Supplier<@Nullable ShareDelegate> shareDelegateSupplier,
            ModalDialogManager modalDialogManager,
            String url,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            StartupMetricsTracker startupMetricsTracker,
            TabModelSelector tabModelSelector,
            OneshotSupplier<ModuleRegistry> moduleRegistrySupplier) {
        // Override surface provider
        Profile profile = Profile.fromWebContents(mTab.getWebContents());
        assertNonNull(profile);

        assert mNewTabPageLayout != null : "Must be already created at NewTabPage.c-tor";
        assert mNewTabPageCoordinator != null : "Must be already created at NewTabPage.c-tor";

        // No-op stub to deal with non-null requirement
        FeedSurfaceCoordinator.ActionDelegateFactory actionDelegate =
                () ->
                        new FeedActionDelegate() {
                            @Override
                            public void openSuggestionUrl(
                                    int disposition,
                                    LoadUrlParams params,
                                    boolean inGroup,
                                    int pageId,
                                    PageLoadObserver pageLoadObserver,
                                    int surfaceId) {
                                assert false : "Not supposed to be invoked";
                            }
                        };

        assertNonNull(mBrowserControlsStateProvider);
        assertNonNull(mToolbarSupplier);
        assertNonNull(mTabStripHeightSupplier);
        assertNonNull(mBottomSheetController);
        assert !FeedFeatures.isFeedEnabled(profile);
        FeedSurfaceCoordinator feedSurfaceCoordinator =
                new BraveFeedSurfaceCoordinator(
                        activity,
                        snackbarManager,
                        windowAndroid,
                        new SnapScrollHelperImpl(mNewTabPageManager, mNewTabPageCoordinator),
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
                        actionDelegate,
                        mTabStripHeightSupplier,
                        edgeToEdgeControllerSupplier,
                        assumeNonNull(moduleRegistrySupplier).get());

        mFeedSurfaceProvider = feedSurfaceCoordinator;
        startupMetricsTracker.registerNtpViewObserver(mFeedSurfaceProvider.getView());
    }

    public void updateSearchProvider() {
        // Search provider logo is not used in Brave's NTP.
        mSearchProviderHasLogo = false;
    }
}
