/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assertNonNull;

import android.app.Activity;
import android.content.Context;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.password_entry_edit.CredentialEditUiFactory;
import org.chromium.chrome.browser.password_entry_edit.CredentialEntryFragmentViewBase;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.search.SettingsSearchCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.Supplier;

@NullMarked
public class BraveFragmentDependencyProvider extends FragmentDependencyProvider {
    private final Profile mProfile;
    private final MonotonicObservableSupplier<ModalDialogManager> mModalDialogManagerSupplier;

    public BraveFragmentDependencyProvider(
            Activity activity,
            Profile profile,
            OneshotSupplier<WindowAndroid> windowAndroidSupplier,
            ActivityResultTracker activityResultTracker,
            OneshotSupplier<SnackbarManager> snackbarManagerSupplier,
            OneshotSupplier<BottomSheetController> bottomSheetControllerSupplier,
            MonotonicObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            Supplier<@Nullable SettingsSearchCoordinator> searchCoordinatorSupplier) {
        super(
                activity,
                profile,
                windowAndroidSupplier,
                activityResultTracker,
                snackbarManagerSupplier,
                bottomSheetControllerSupplier,
                modalDialogManagerSupplier,
                searchCoordinatorSupplier);
        mProfile = profile;
        mModalDialogManagerSupplier = modalDialogManagerSupplier;
    }

    @Override
    public void onFragmentAttached(
            FragmentManager fragmentManager, Fragment fragment, Context unusedContext) {
        if (fragment instanceof CredentialEntryFragmentViewBase credentialFragment) {
            CredentialEditUiFactory.create(
                    credentialFragment, mProfile, assertNonNull(mModalDialogManagerSupplier.get()));
        }

        super.onFragmentAttached(fragmentManager, fragment, unusedContext);
    }
}
