/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreferenceCompat;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("chrome::android")
public class DesktopModePreferences
        extends PreferenceFragmentCompat implements Preference.OnPreferenceChangeListener {
    public static final String DESKTOP_MODE_KEY = "desktop_mode";
    private Profile mProfile;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.settings_desktop_mode_title);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.desktop_mode_preferences);

        ChromeSwitchPreferenceCompat desktopModePref =
                (ChromeSwitchPreferenceCompat) findPreference(DESKTOP_MODE_KEY);
        desktopModePref.setChecked(DesktopModePreferencesJni.get().getDesktopModeEnabled(getProfile()));
        desktopModePref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        DesktopModePreferencesJni.get().setDesktopModeEnabled((boolean) newValue, getProfile());
        return true;
    }

    private Profile getProfile() {
        if (mProfile == null)
            mProfile = Profile.getLastUsedProfile();
        return mProfile;
    }

    @NativeMethods
    interface Natives {
        void setDesktopModeEnabled(boolean enabled, Profile profile);
        boolean getDesktopModeEnabled(Profile profile);
    }
}
