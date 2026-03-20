/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.feed.FeedSurfaceScrollDelegate;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.url.GURL;

import java.util.function.Supplier;

@NullMarked
public class BraveNewTabPageCoordinator extends NewTabPageCoordinator {
    private static final String TAG = "BraveNewTabPageCoordinator";

    private final Activity mActivity;

    private final BraveNewTabPageLayout mBraveNewTabPageLayout;
    private final NewTabPageManager mNewTabPageManager;

    public BraveNewTabPageCoordinator(
            NewTabPageManager manager, Activity activity, NewTabPageLayout newTabPageLayout) {
        super(manager, activity, newTabPageLayout);

        mNewTabPageManager = manager;

        assert (activity instanceof BraveActivity);
        mActivity = activity;

        assert (newTabPageLayout instanceof BraveNewTabPageLayout);
        mBraveNewTabPageLayout = (BraveNewTabPageLayout) newTabPageLayout;
    }

    @Override
    @Initializer
    public void initialize(
            TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo,
            boolean searchProviderIsGoogle,
            FeedSurfaceScrollDelegate scrollDelegate,
            TouchEnabledDelegate touchEnabledDelegate,
            UiConfig uiConfig,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            Profile profile,
            WindowAndroid windowAndroid,
            ActivityResultTracker activityResultTracker,
            BottomSheetController bottomSheetController,
            ModalDialogManager modalDialogManager,
            SnackbarManager snackbarManager,
            boolean isTablet,
            Supplier<Integer> tabStripHeightSupplier,
            Supplier<GURL> composeplateUrlSupplier) {
        super.initialize(
                tileGroupDelegate,
                searchProviderHasLogo,
                searchProviderIsGoogle,
                scrollDelegate,
                touchEnabledDelegate,
                uiConfig,
                lifecycleDispatcher,
                profile,
                windowAndroid,
                activityResultTracker,
                bottomSheetController,
                modalDialogManager,
                snackbarManager,
                isTablet,
                tabStripHeightSupplier,
                composeplateUrlSupplier);

        mBraveNewTabPageLayout.initialize(mNewTabPageManager, mActivity, profile, windowAndroid);
    }
}
