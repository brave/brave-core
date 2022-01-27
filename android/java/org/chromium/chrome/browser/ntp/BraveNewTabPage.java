/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import androidx.annotation.Nullable;

import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.feed.BraveFeedSurfaceCoordinator;
import org.chromium.chrome.browser.feed.FeedFeatures;
import org.chromium.chrome.browser.feed.FeedLaunchReliabilityLoggingState;
import org.chromium.chrome.browser.feed.FeedSurfaceProvider;
import org.chromium.chrome.browser.feed.FeedSwipeRefreshLayout;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncherImpl;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.xsurface.FeedLaunchReliabilityLogger.SurfaceType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.base.WindowAndroid;

public class BraveNewTabPage extends NewTabPage {
    // To delete in bytecode, members from parent class will be used instead.
    private BrowserControlsStateProvider mBrowserControlsStateProvider;
    private NewTabPageLayout mNewTabPageLayout;
    private FeedSurfaceProvider mFeedSurfaceProvider;
    private Supplier<Toolbar> mToolbarSupplier;

    public BraveNewTabPage(Activity activity,
            BrowserControlsStateProvider browserControlsStateProvider,
            Supplier<Tab> activityTabProvider, SnackbarManager snackbarManager,
            ActivityLifecycleDispatcher lifecycleDispatcher, TabModelSelector tabModelSelector,
            boolean isTablet, NewTabPageUma uma, boolean isInNightMode,
            NativePageHost nativePageHost, Tab tab, String url,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier, WindowAndroid windowAndroid,
            JankTracker jankTracker, Supplier<Toolbar> toolbarSupplier) {
        super(activity, browserControlsStateProvider, activityTabProvider, snackbarManager,
                lifecycleDispatcher, tabModelSelector, isTablet, uma, isInNightMode, nativePageHost,
                tab, url, bottomSheetController, shareDelegateSupplier, windowAndroid, jankTracker,
                toolbarSupplier);

        assert mNewTabPageLayout instanceof BraveNewTabPageLayout;
        if (mNewTabPageLayout instanceof BraveNewTabPageLayout) {
            ((BraveNewTabPageLayout) mNewTabPageLayout).setTab(tab);
        }
    }

    @Override
    protected void initializeMainView(Activity activity, WindowAndroid windowAndroid,
            SnackbarManager snackbarManager, NewTabPageUma uma, boolean isInNightMode,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier, String url) {
        super.initializeMainView(activity, windowAndroid, snackbarManager, uma, isInNightMode,
                bottomSheetController, shareDelegateSupplier, url);
        // Override surface provider
        Profile profile = Profile.fromWebContents(mTab.getWebContents());

        assert !FeedFeatures.isFeedEnabled();
        mFeedSurfaceProvider = new BraveFeedSurfaceCoordinator(activity, snackbarManager,
                windowAndroid, new SnapScrollHelperImpl(mNewTabPageManager, mNewTabPageLayout),
                mNewTabPageLayout, mBrowserControlsStateProvider.getTopControlsHeight(),
                isInNightMode, this, profile,
                /* isPlaceholderShownInitially= */ false, bottomSheetController,
                shareDelegateSupplier, /* externalScrollableContainerDelegate= */ null,
                NewTabPageUtils.decodeOriginFromNtpUrl(url),
                PrivacyPreferencesManagerImpl.getInstance(), mToolbarSupplier,
                new FeedLaunchReliabilityLoggingState(SurfaceType.NEW_TAB_PAGE, mConstructedTimeNs),
                FeedSwipeRefreshLayout.create(activity, R.id.toolbar_container),
                /* overScrollDisabled= */ false, /* viewportView= */ null,
                /* actionDelegate= */ null, HelpAndFeedbackLauncherImpl.getInstance());
    }
}
