/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.customtabs.FullScreenCustomTabActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

/** Owns the Email Aliases row in the main settings screen. */
@NullMarked
public class BraveEmailAliasesSectionController {
    public static final String PREF_EMAIL_ALIASES = "email_aliases";

    private static final String EMAIL_ALIASES_URL = "brave://email-aliases";
    private static final String EMAIL_ALIASES_ENABLED_PREF = "brave.email_aliases.enabled";

    private final ChromeBaseSettingsFragment mFragment;

    public static @Nullable BraveEmailAliasesSectionController maybeCreate(
            ChromeBaseSettingsFragment fragment) {
        return ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)
                ? new BraveEmailAliasesSectionController(fragment)
                : null;
    }

    private BraveEmailAliasesSectionController(ChromeBaseSettingsFragment fragment) {
        mFragment = fragment;

        Preference preference = mFragment.findPreference(PREF_EMAIL_ALIASES);
        if (preference != null) {
            preference.setOnPreferenceClickListener(unused -> openEmailAliases());
        }
    }

    public static boolean isEnabled(Profile profile) {
        return ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)
                && UserPrefs.get(profile).getBoolean(EMAIL_ALIASES_ENABLED_PREF);
    }

    public void updateVisibility() {
        Preference preference = mFragment.findPreference(PREF_EMAIL_ALIASES);
        if (preference != null) {
            preference.setVisible(isEnabled(mFragment.getProfile()));
        }
    }

    private boolean openEmailAliases() {
        Activity activity = mFragment.getActivity();
        if (activity == null || activity.isFinishing()) {
            return false;
        }

        FullScreenCustomTabActivity.showPage(activity, EMAIL_ALIASES_URL);
        return true;
    }
}
