/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import com.google.android.material.materialswitch.MaterialSwitch;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ProfileDependentSetting;
import org.chromium.components.browser_ui.settings.EmbeddableSettingsPage;
import org.chromium.components.browser_ui.settings.SettingsFragment.AnimationType;

/**
 * Settings screen for the browser-wide biometric lock. Each lock target (entire application,
 * private tabs only) has its own independent toggle — both can be enabled simultaneously.
 *
 * <p>This fragment is only reachable when a device screen lock is configured; the main settings
 * item redirects to OS security settings instead when none is set up.
 */
@NullMarked
public class BraveBrowserLockSettingsFragment extends Fragment
        implements EmbeddableSettingsPage, ProfileDependentSetting {

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    private @Nullable Profile mProfile;

    @Override
    public void setProfile(Profile profile) {
        mProfile = profile;
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        mPageTitle.set(getString(R.string.brave_browser_lock_title));
        return inflater.inflate(R.layout.fragment_brave_browser_lock_settings, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        MaterialSwitch switchEntireApp = view.findViewById(R.id.switch_entire_application);
        MaterialSwitch switchPrivateTabs = view.findViewById(R.id.switch_private_tabs);

        assumeNonNull(switchEntireApp);
        assumeNonNull(switchPrivateTabs);

        boolean available =
                IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                        && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled();
        switchEntireApp.setEnabled(available);
        switchPrivateTabs.setEnabled(available);

        switchEntireApp.setChecked(isEntireAppEnabled());
        switchPrivateTabs.setChecked(isPrivateTabsEnabled());

        switchEntireApp.setOnCheckedChangeListener(
                (buttonView, isChecked) ->
                        onToggleChanged(
                                BravePreferenceKeys.BRAVE_BROWSER_LOCK,
                                switchEntireApp,
                                isChecked));
        switchPrivateTabs.setOnCheckedChangeListener(
                (buttonView, isChecked) ->
                        onToggleChanged(
                                BravePreferenceKeys.BRAVE_BROWSER_LOCK_PRIVATE_TABS_ONLY,
                                switchPrivateTabs,
                                isChecked));
    }

    private void onToggleChanged(String prefKey, MaterialSwitch toggle, boolean isChecked) {
        Profile profile = mProfile;
        if (profile == null) return;
        boolean previous = ChromeSharedPreferences.getInstance().readBoolean(prefKey, false);
        if (previous == isChecked) return;

        IncognitoReauthManager reauth = new IncognitoReauthManager(requireActivity(), profile);
        reauth.startReauthenticationFlow(
                new IncognitoReauthManager.IncognitoReauthCallback() {
                    @Override
                    public void onIncognitoReauthNotPossible() {
                        reauth.destroy();
                        revertToggle(toggle, prefKey, previous);
                    }

                    @Override
                    public void onIncognitoReauthSuccess() {
                        reauth.destroy();
                        ChromeSharedPreferences.getInstance().writeBoolean(prefKey, isChecked);
                    }

                    @Override
                    public void onIncognitoReauthFailure() {
                        reauth.destroy();
                        revertToggle(toggle, prefKey, previous);
                    }
                });
    }

    private void revertToggle(MaterialSwitch toggle, String prefKey, boolean previousValue) {
        if (!isAdded()) {
            return;
        }
        toggle.setOnCheckedChangeListener(null);
        toggle.setChecked(previousValue);
        toggle.setOnCheckedChangeListener(
                (buttonView, isChecked) -> onToggleChanged(prefKey, toggle, isChecked));
    }

    private static boolean isEntireAppEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    private static boolean isPrivateTabsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK_PRIVATE_TABS_ONLY, false);
    }
}
