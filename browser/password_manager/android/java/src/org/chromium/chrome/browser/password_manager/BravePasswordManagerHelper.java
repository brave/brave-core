// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager;

import android.content.Context;
import android.os.Bundle;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileKeyedMap;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.settings.SettingsCustomTabLauncher;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.Supplier;

@NullMarked
public class BravePasswordManagerHelper extends PasswordManagerHelper {
    private static final String TAG = "BravePasswords";
    protected static @Nullable ProfileKeyedMap<PasswordManagerHelper> sProfileMap;

    // Key for the argument with which PasswordsSettings will be launched. The value for
    // this argument should be part of the ManagePasswordsReferrer enum, which contains
    // all points of entry to the passwords settings.
    public static final String MANAGE_PASSWORDS_REFERRER = "manage-passwords-referrer";

    BravePasswordManagerHelper(Profile profile) {
        super(profile);
    }

    @Override
    public void showPasswordSettings(
            Context context,
            @ManagePasswordsReferrer int referrer,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            boolean managePasskeys,
            @Nullable String account,
            SettingsCustomTabLauncher settingsCustomTabLauncher) {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putInt(MANAGE_PASSWORDS_REFERRER, referrer);

        // PasswordSettings is nota available at current package.
        // Upstream used to resolve it through SettingsNavigation.SettingsFragment.PASSWORDS
        // enum. To avoid use patching, just use reflection, luckily we have
        // settingsNavigation.createSettingsIntent which can accept Class argument.
        try {
            Class passwordSettingsClass =
                    Class.forName(
                            "org.chromium.chrome.browser.password_manager.settings.PasswordSettings"); // presubmit: ignore-long-line
            context.startActivity(
                    SettingsNavigationFactory.createSettingsNavigation()
                            .createSettingsIntent(context, passwordSettingsClass, fragmentArgs));
        } catch (Exception e) {
            Log.e(TAG, "showPasswordSettings: ", e);
        }
    }

    public static PasswordManagerHelper getForProfile(Profile profile) {
        if (sProfileMap == null) {
            sProfileMap =
                    new ProfileKeyedMap<>(
                            ProfileKeyedMap.ProfileSelection.REDIRECTED_TO_ORIGINAL,
                            ProfileKeyedMap.NO_REQUIRED_CLEANUP_ACTION);
        }
        return sProfileMap.getForProfile(profile, BravePasswordManagerHelper::new);
    }
}
