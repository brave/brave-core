/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.password_entry_edit.CredentialEditUiFactory;
import org.chromium.chrome.browser.password_entry_edit.CredentialEntryFragmentViewBase;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.ui.modaldialog.ModalDialogManager;

@NullMarked
public class BraveFragmentDependencyProvider extends FragmentDependencyProvider {
    private final Profile mProfile;

    public BraveFragmentDependencyProvider(
            Context context,
            Profile profile,
            OneshotSupplier<SnackbarManager> snackbarManagerSupplier,
            OneshotSupplier<BottomSheetController> bottomSheetControllerSupplier,
            ObservableSupplier<ModalDialogManager> modalDialogManagerSupplier) {
        super(
                context,
                profile,
                snackbarManagerSupplier,
                bottomSheetControllerSupplier,
                modalDialogManagerSupplier);
        mProfile = profile;
    }

    @Override
    public void onFragmentAttached(
            FragmentManager fragmentManager, Fragment fragment, Context unusedContext) {
        if (fragment instanceof CredentialEntryFragmentViewBase) {
            CredentialEditUiFactory.create((CredentialEntryFragmentViewBase) fragment, mProfile);
        }

        super.onFragmentAttached(fragmentManager, fragment, unusedContext);
    }
}
