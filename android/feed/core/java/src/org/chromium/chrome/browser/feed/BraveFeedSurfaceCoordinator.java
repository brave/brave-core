/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.feed;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Px;

import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NewTabPageLaunchOrigin;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.toolbar.top.Toolbar;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.WindowAndroid;

public class BraveFeedSurfaceCoordinator extends FeedSurfaceCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private View mNtpHeader;
    private FrameLayout mRootView;

    // Own members.
    private @Nullable FrameLayout mFrameLayoutForPolicy;

    public BraveFeedSurfaceCoordinator(
            Activity activity,
            SnackbarManager snackbarManager,
            WindowAndroid windowAndroid,
            @Nullable JankTracker jankTracker,
            @Nullable SnapScrollHelper snapScrollHelper,
            @Nullable View ntpHeader,
            @Px int toolbarHeight,
            boolean showDarkBackground,
            FeedSurfaceDelegate delegate,
            Profile profile,
            BottomSheetController bottomSheetController,
            Supplier<ShareDelegate> shareDelegateSupplier,
            @Nullable ScrollableContainerDelegate externalScrollableContainerDelegate,
            @NewTabPageLaunchOrigin int launchOrigin,
            PrivacyPreferencesManagerImpl privacyPreferencesManager,
            @NonNull Supplier<Toolbar> toolbarSupplier,
            long embeddingSurfaceCreatedTimeNs,
            @Nullable FeedSwipeRefreshLayout swipeRefreshLayout,
            boolean overScrollDisabled,
            @Nullable ViewGroup viewportView,
            FeedActionDelegate actionDelegate,
            @NonNull ObservableSupplier<Integer> tabStripHeightSupplier,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier) {
        super(
                activity,
                snackbarManager,
                windowAndroid,
                jankTracker,
                snapScrollHelper,
                ntpHeader,
                toolbarHeight,
                showDarkBackground,
                delegate,
                profile,
                bottomSheetController,
                shareDelegateSupplier,
                externalScrollableContainerDelegate,
                launchOrigin,
                privacyPreferencesManager,
                toolbarSupplier,
                embeddingSurfaceCreatedTimeNs,
                swipeRefreshLayout,
                overScrollDisabled,
                viewportView,
                actionDelegate,
                tabStripHeightSupplier,
                edgeToEdgeControllerSupplier);
    }

    public void createFrameLayoutForPolicy() {
        assert mFrameLayoutForPolicy == null : "mFrameLayoutForPolicy should be created only once!";

        // Remove all previously added views.
        mRootView.removeAllViews();

        mFrameLayoutForPolicy = new FrameLayout(mActivity);
        mFrameLayoutForPolicy.setLayoutParams(
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));
        mFrameLayoutForPolicy.setBackgroundColor(
                mActivity.getColor(R.color.default_bg_color_baseline));

        // Make framelayout focusable so that it is the next focusable view when the url bar clears
        // focus.
        mFrameLayoutForPolicy.setFocusable(true);
        mFrameLayoutForPolicy.setFocusableInTouchMode(true);
        mFrameLayoutForPolicy.setContentDescription(mFrameLayoutForPolicy.getResources().getString(
                R.string.accessibility_new_tab_page));

        if (mNtpHeader != null) {
            UiUtils.removeViewFromParent(mNtpHeader);
            mFrameLayoutForPolicy.addView(mNtpHeader);
        }
        mRootView.addView(mFrameLayoutForPolicy);
    }

    public FrameLayout getFrameLayoutForPolicy() {
        return mFrameLayoutForPolicy;
    }
}
