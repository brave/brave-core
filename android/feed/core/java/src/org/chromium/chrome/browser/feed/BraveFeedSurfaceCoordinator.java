/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.feed;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ScrollView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Px;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.feed.hooks.FeedHooks;
import org.chromium.chrome.browser.feed.sections.SectionHeaderView;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncher;
import org.chromium.chrome.browser.native_page.NativePageNavigationDelegate;
import org.chromium.chrome.browser.ntp.NewTabPageLaunchOrigin;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.WindowAndroid;

public class BraveFeedSurfaceCoordinator extends FeedSurfaceCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private @Nullable ScrollView mScrollViewForPolicy;
    private View mNtpHeader;
    private FrameLayout mRootView;

    public BraveFeedSurfaceCoordinator(Activity activity, SnackbarManager snackbarManager,
            WindowAndroid windowAndroid, @Nullable SnapScrollHelper snapScrollHelper,
            @Nullable View ntpHeader, @Px int toolbarHeight, boolean showDarkBackground,
            FeedSurfaceDelegate delegate, Profile profile, boolean isPlaceholderShownInitially,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier,
            @Nullable ScrollableContainerDelegate externalScrollableContainerDelegate,
            @NewTabPageLaunchOrigin int launchOrigin,
            PrivacyPreferencesManagerImpl privacyPreferencesManager,
            @NonNull Supplier<Toolbar> toolbarSupplier,
            FeedLaunchReliabilityLoggingState launchReliabilityLoggingState,
            @Nullable FeedSwipeRefreshLayout swipeRefreshLayout, boolean overScrollDisabled,
            @Nullable ViewGroup viewportView, FeedActionDelegate actionDelegate,
            HelpAndFeedbackLauncher helpAndFeedbackLauncher) {
        super(activity, snackbarManager, windowAndroid, snapScrollHelper, ntpHeader, toolbarHeight,
                showDarkBackground, delegate, profile, isPlaceholderShownInitially,
                bottomSheetController, shareDelegateSupplier, externalScrollableContainerDelegate,
                launchOrigin, privacyPreferencesManager, toolbarSupplier,
                launchReliabilityLoggingState, swipeRefreshLayout, overScrollDisabled, viewportView,
                actionDelegate, helpAndFeedbackLauncher);
    }

    public boolean isEnhancedProtectionPromoEnabled() {
        return false;
    }

    public boolean isReliabilityLoggingEnabled() {
        return false;
    }
}
