/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.browser.flags.ChromeFeatureList;

public class BraveSettingsIntentUtil {
    private static String PASSWORD_SETTINGS_FRAGMENT =
            "org.chromium.chrome.browser.password_manager.settings.PasswordSettings";
    private static String CREDENTIAL_EDIT_FRAGMENT =
            "org.chromium.chrome.browser.password_entry_edit.CredentialEditFragmentView";

    public static Intent createIntent(
            @NonNull Context context,
            @Nullable String fragmentName,
            @Nullable Bundle fragmentArgs) {
        Intent intent = SettingsIntentUtil.createIntent(context, fragmentName, fragmentArgs);
        intent.setClass(context, BraveSettingsActivity.class);
        /*
         * Password settings and credential edit fragments are not used in the upstream anymore and
         * thus not adjusted to be used within single Settings activity. For now we just open them
         * with a separate activity as it used to be.
         * Going forward we should adjust them to be used within the single Settings activity
         * https://github.com/brave/brave-browser/issues/41977
         */
        if (ChromeFeatureList.sSettingsSingleActivity.isEnabled()
                && fragmentName != null
                && (fragmentName.equals(PASSWORD_SETTINGS_FRAGMENT)
                        || fragmentName.equals(CREDENTIAL_EDIT_FRAGMENT))) {
            intent.removeFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        }
        return intent;
    }
}
