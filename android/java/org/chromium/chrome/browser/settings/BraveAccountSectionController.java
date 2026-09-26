/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;

import androidx.preference.Preference;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.components.brave_account.BraveAccountFeatures;

/**
 * Owns the Brave Account row in the main settings screen. The row is static: the account state it
 * used to render natively - verification, password change, sign out, deletion - is served by the
 * Brave Account WebUI the row opens.
 */
@NullMarked
public class BraveAccountSectionController {
    public static final String PREF_BRAVE_ACCOUNT = "brave_account";

    private final ChromeBaseSettingsFragment mFragment;

    public static @Nullable BraveAccountSectionController maybeCreate(
            ChromeBaseSettingsFragment fragment) {
        return BraveAccountFeatures.isBraveAccountEnabled()
                ? new BraveAccountSectionController(fragment)
                : null;
    }

    private BraveAccountSectionController(ChromeBaseSettingsFragment fragment) {
        mFragment = fragment;

        Preference preference = mFragment.findPreference(PREF_BRAVE_ACCOUNT);
        if (preference != null) {
            preference.setOnPreferenceClickListener(unused -> openBraveAccountSettings());
        }
    }

    private boolean openBraveAccountSettings() {
        if (!mFragment.isAdded() || mFragment.isDetached()) {
            return false;
        }

        Activity activity = mFragment.getActivity();
        if (activity == null || activity.isFinishing()) {
            return false;
        }

        BraveAccountCustomTabActivity.openBraveAccountSettings(activity);
        return true;
    }
}
