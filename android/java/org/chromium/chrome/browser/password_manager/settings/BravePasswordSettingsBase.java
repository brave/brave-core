/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.password_manager.settings;

import android.view.Menu;

import org.chromium.chrome.browser.settings.BravePreferenceFragment;

public abstract class BravePasswordSettingsBase extends BravePreferenceFragment {
    /*
     * This variable will be used instead of upstream's `PasswordSettings#mMenu`.
     */
    protected Menu mMenu;

    public void createCheckPasswords() {
        // Do nothing here as we don't have check passwords option in Brave.
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        /*
         * In `PasswordSettings` class `passwordListAvailable` method gets called before `onCreateOptionsMenu`.
         * Now on pressing `back`, Seetings activity is not finished, but just returns 1 fragment back.
         * This causes null pointer crash on returning to the password settings page, since `mMenu` is already not null,
         * but not inflated yet by the time we call `passwordListAvailable` and we try to access `export_passwords`.
         * This issue is not happening in the upstream since they use different password settings page that utilizes OS level password manager.
         * So here we make sure `mMenu` is null every time we leave password settings page.
         */
        mMenu = null;
    }
}
