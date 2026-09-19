/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;

import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.price_tracking.PriceDropNotificationManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.ui.signin.SigninAndHistorySyncActivityLauncher;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.device_lock.DeviceLockActivityLauncher;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;

import java.util.function.Supplier;

public class BraveBookmarkPage extends BookmarkPage {
    // Overridden Chromium's BookmarkPage.mManager
    private BookmarkManagerCoordinator mBookmarkManagerCoordinator;

    public BraveBookmarkPage(
            WindowAndroid windowAndroid,
            Activity activity,
            SnackbarManager snackbarManager,
            Supplier<BottomSheetController> bottomSheetControllerSupplier,
            ActivityResultTracker activityResultTracker,
            Profile profile,
            NativePageHost host,
            BookmarkOpener bookmarkOpener,
            BookmarkManagerOpener bookmarkManagerOpener,
            PriceDropNotificationManager priceDropNotificationManager,
            SigninAndHistorySyncActivityLauncher signinAndHistorySyncActivityLauncher,
            DeviceLockActivityLauncher deviceLockActivityLauncher,
            BackPressManager backPressManager) {
        super(
                windowAndroid,
                activity,
                snackbarManager,
                bottomSheetControllerSupplier,
                activityResultTracker,
                profile,
                host,
                bookmarkOpener,
                bookmarkManagerOpener,
                priceDropNotificationManager,
                signinAndHistorySyncActivityLauncher,
                deviceLockActivityLauncher,
                backPressManager);

        if (mBookmarkManagerCoordinator instanceof BraveBookmarkManagerCoordinator
                && BraveActivity.getChromeTabbedActivity() != null) {
            ((BraveBookmarkManagerCoordinator) mBookmarkManagerCoordinator)
                    .setWindow(BraveActivity.getChromeTabbedActivity().getWindowAndroid());
        }
    }
}
