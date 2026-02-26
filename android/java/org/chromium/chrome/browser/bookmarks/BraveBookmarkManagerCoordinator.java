/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;
import android.view.View;

import androidx.annotation.Nullable;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.price_tracking.PriceDropNotificationManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.edge_to_edge.EdgeToEdgePadAdjuster;

import java.util.function.Function;
import java.util.function.Supplier;

@NullMarked
public class BraveBookmarkManagerCoordinator extends BookmarkManagerCoordinator {
    // Overridden Chromium's BookmarkManagerCoordinator.mMediator
    private @Nullable BookmarkManagerMediator mMediator;

    public BraveBookmarkManagerCoordinator(
            WindowAndroid windowAndroid,
            Activity activity,
            boolean isDialogUi,
            SnackbarManager snackbarManager,
            Supplier<BottomSheetController> bottomSheetControllerSupplier,
            ActivityResultTracker activityResultTracker,
            Profile profile,
            BookmarkUiPrefs bookmarkUiPrefs,
            BookmarkOpener bookmarkOpener,
            BookmarkManagerOpener bookmarkManagerOpener,
            PriceDropNotificationManager priceDropNotificationManager,
            @Nullable Function<View, EdgeToEdgePadAdjuster> edgeToEdgePadAdjusterGenerator,
            @Nullable BackPressManager backPressManager) {
        super(
                windowAndroid,
                activity,
                isDialogUi,
                snackbarManager,
                bottomSheetControllerSupplier,
                activityResultTracker,
                profile,
                bookmarkUiPrefs,
                bookmarkOpener,
                bookmarkManagerOpener,
                priceDropNotificationManager,
                edgeToEdgePadAdjusterGenerator,
                backPressManager);
    }

    public void setWindow(ActivityWindowAndroid window) {
        if (mMediator instanceof BraveBookmarkManagerMediator) {
            ((BraveBookmarkManagerMediator) mMediator).setWindow(window);
        }
    }
}
