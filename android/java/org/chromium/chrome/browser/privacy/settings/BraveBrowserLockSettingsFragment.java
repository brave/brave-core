/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

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
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ProfileDependentSetting;
import org.chromium.components.browser_ui.settings.EmbeddableSettingsPage;
import org.chromium.components.browser_ui.settings.SettingsFragment.AnimationType;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Settings screen for the browser-wide biometric lock. Each lock target (entire application,
 * private tabs only) has its own independent toggle — both can be enabled simultaneously.
 *
 * <p>The "Private tabs" toggle is backed by Chromium's own {@link
 * Pref#INCOGNITO_REAUTHENTICATION_FOR_ANDROID} pref (the same pref driving {@link
 * org.chromium.chrome.browser.incognito.reauth.IncognitoReauthControllerImpl}), not a Brave-only
 * pref — this is the pref that actually gates per-tab incognito reauth.
 *
 * <p>A third toggle, "Prevent screenshot/video capture", only appears once at least one lock target
 * is enabled, and blocks screen capture for whatever is currently locked down.
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

        assert switchEntireApp != null;
        assert switchPrivateTabs != null;

        boolean available =
                IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                        && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled();
        switchEntireApp.setEnabled(available);
        switchPrivateTabs.setEnabled(available);

        Profile profile = mProfile;
        switchEntireApp.setChecked(isEntireAppEnabled());
        switchPrivateTabs.setChecked(profile != null && isPrivateTabsEnabled(profile));
        switchPreventCapture.setChecked(isPreventCaptureEnabled());
        updatePreventCaptureVisibility();

        switchEntireApp.setOnCheckedChangeListener(
                (buttonView, isChecked) ->
                        onToggleChanged(
                                BravePreferenceKeys.BRAVE_BROWSER_LOCK,
                                switchEntireApp,
                                isChecked));
        switchPrivateTabs.setOnCheckedChangeListener(
                (buttonView, isChecked) ->
                        onPrivateTabsToggleChanged(switchPrivateTabs, isChecked));
        switchPreventCapture.setOnCheckedChangeListener(
                (buttonView, isChecked) ->
                        onToggleChanged(
                                BravePreferenceKeys.BRAVE_BROWSER_LOCK_PREVENT_CAPTURE,
                                switchPreventCapture,
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
        toggle.setOnCheckedChangeListener(null);
        toggle.setChecked(previousValue);
        toggle.setOnCheckedChangeListener(
                (buttonView, isChecked) -> onToggleChanged(prefKey, toggle, isChecked));
        updatePreventCaptureVisibility();
    }

    private void onPrivateTabsToggleChanged(MaterialSwitch toggle, boolean isChecked) {
        Profile profile = mProfile;
        if (profile == null) return;
        boolean previous = isPrivateTabsEnabled(profile);
        if (previous == isChecked) return;

        IncognitoReauthManager reauth = new IncognitoReauthManager(requireActivity(), profile);
        reauth.startReauthenticationFlow(
                new IncognitoReauthManager.IncognitoReauthCallback() {
                    @Override
                    public void onIncognitoReauthNotPossible() {
                        reauth.destroy();
                        revertPrivateTabsToggle(toggle, previous);
                    }

                    @Override
                    public void onIncognitoReauthSuccess() {
                        reauth.destroy();
                        UserPrefs.get(profile)
                                .setBoolean(Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID, isChecked);
                        updatePreventCaptureVisibility();
                    }

                    @Override
                    public void onIncognitoReauthFailure() {
                        reauth.destroy();
                        revertPrivateTabsToggle(toggle, previous);
                    }
                });
    }

    private void revertPrivateTabsToggle(MaterialSwitch toggle, boolean previousValue) {
        if (!isAdded()) {
            return;
        }
        toggle.setOnCheckedChangeListener(null);
        toggle.setChecked(previousValue);
        toggle.setOnCheckedChangeListener(
                (buttonView, isChecked) -> onPrivateTabsToggleChanged(toggle, isChecked));
        updatePreventCaptureVisibility();
    }

    private void updatePreventCaptureVisibility() {
        View container = mPreventCaptureContainer;
        if (container == null) return;
        Profile profile = mProfile;
        boolean anyLocked =
                isEntireAppEnabled() || (profile != null && isPrivateTabsEnabled(profile));
        container.setVisibility(anyLocked ? View.VISIBLE : View.GONE);
    }

    private static boolean defaultForPref(String prefKey) {
        return BravePreferenceKeys.BRAVE_BROWSER_LOCK_PREVENT_CAPTURE.equals(prefKey);
    }

    private static boolean isEntireAppEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    private static boolean isPrivateTabsEnabled(Profile profile) {
        return UserPrefs.get(profile).getBoolean(Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID);
    }
}
