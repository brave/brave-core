/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import android.app.Activity;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/**
 * Manages the browser lock toggle in Privacy and Security settings. Requires biometric
 * re-authentication before the setting can be changed, matching the pattern in {@link
 * IncognitoLockSettings}.
 */
@NullMarked
class BraveLockSettings {
    private final ChromeSwitchPreference mPreference;
    private final Profile mProfile;

    private boolean mIsChromeTriggered;
    private @Nullable IncognitoReauthManager mReauthManager;

    BraveLockSettings(ChromeSwitchPreference preference, Profile profile) {
        mPreference = preference;
        mProfile = profile;
    }

    void setUp(Activity activity) {
        if (!IncognitoReauthManager.isIncognitoReauthFeatureAvailable()) {
            mPreference.setVisible(false);
            return;
        }
        mPreference.setEnabled(IncognitoReauthSettingUtils.isDeviceScreenLockEnabled());
        mPreference.setChecked(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false));
        mPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    onPreferenceChange(activity, (boolean) newValue);
                    return true;
                });
    }

    void destroy() {
        if (mReauthManager != null) {
            mReauthManager.destroy();
            mReauthManager = null;
        }
    }

    private void onPreferenceChange(Activity activity, boolean newValue) {
        if (mIsChromeTriggered) return;
        boolean previousValue =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
        if (mReauthManager == null) {
            mReauthManager = new IncognitoReauthManager(activity, mProfile);
        }
        mReauthManager.startReauthenticationFlow(
                new IncognitoReauthManager.IncognitoReauthCallback() {
                    @Override
                    public void onIncognitoReauthNotPossible() {
                        revertToggle(previousValue);
                    }

                    @Override
                    public void onIncognitoReauthSuccess() {
                        ChromeSharedPreferences.getInstance()
                                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, newValue);
                    }

                    @Override
                    public void onIncognitoReauthFailure() {
                        revertToggle(previousValue);
                    }
                });
    }

    private void revertToggle(boolean previousValue) {
        mIsChromeTriggered = true; // Ensure callback bails early.
        mPreference.setChecked(previousValue);
        mIsChromeTriggered = false;
    }
}
