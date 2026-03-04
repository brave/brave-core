/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.identity_disc;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.device_lock.DeviceLockActivityLauncher;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Brave's implementation for IdentityDiscController. */
public class BraveIdentityDiscController extends IdentityDiscController {
    public BraveIdentityDiscController(
            Activity activity,
            WindowAndroid windowAndroid,
            ActivityResultTracker activityResultTracker,
            DeviceLockActivityLauncher deviceLockActivityLauncher,
            MonotonicObservableSupplier<Profile> profileSupplier,
            BottomSheetController bottomSheetController,
            ModalDialogManager modalDialogManager,
            SnackbarManager snackbarManager) {
        super(
                activity,
                windowAndroid,
                activityResultTracker,
                deviceLockActivityLauncher,
                profileSupplier,
                bottomSheetController,
                modalDialogManager,
                snackbarManager);
    }

    /*
     * We want to override `IdentityDiscController#calculateButtonData` via asm
     * to avoid enabling identity button on the home page
     * as this button only meant to be used with Google account.
     */
    public void calculateButtonData() {}
}
