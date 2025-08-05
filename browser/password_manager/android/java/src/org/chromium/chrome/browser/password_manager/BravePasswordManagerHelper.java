// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager;

import android.content.Context;
import android.os.Bundle;

import org.chromium.base.supplier.Supplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileKeyedMap;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.settings.SettingsCustomTabLauncher;
import org.chromium.components.browser_ui.settings.SettingsNavigation.SettingsFragment;
import org.chromium.ui.modaldialog.ModalDialogManager;

@NullMarked
public class BravePasswordManagerHelper extends PasswordManagerHelper {

    protected static @Nullable ProfileKeyedMap<PasswordManagerHelper> sProfileMap;

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
        context.startActivity(
                SettingsNavigationFactory.createSettingsNavigation()
                        .createSettingsIntent(context, SettingsFragment.PASSWORDS, fragmentArgs));
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
