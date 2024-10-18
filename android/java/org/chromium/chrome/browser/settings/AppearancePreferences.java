/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.settings.MainSettings.PREF_UI_THEME;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowDialogFragment;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowUtils;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.night_mode.NightModeUtils;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.DeviceFormFactor;

public class AppearancePreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener, BraveRewardsObserver {
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON = "hide_brave_rewards_icon";
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON_MIGRATION =
            "hide_brave_rewards_icon_migration";
    public static final String PREF_SHOW_BRAVE_REWARDS_ICON = "show_brave_rewards_icon";
    public static final String PREF_ADS_SWITCH = "ads_switch";
    public static final String PREF_BRAVE_NIGHT_MODE_ENABLED = "brave_night_mode_enabled_key";
    public static final String PREF_BRAVE_DISABLE_SHARING_HUB = "brave_disable_sharing_hub";
    public static final String PREF_BRAVE_ENABLE_TAB_GROUPS = "brave_enable_tab_groups";
    public static final String PREF_BRAVE_ENABLE_SPEEDREADER = "brave_enable_speedreader";
    public static final String PREF_ENABLE_MULTI_WINDOWS = "enable_multi_windows";
    public static final String PREF_SHOW_UNDO_WHEN_TABS_CLOSED = "show_undo_when_tabs_closed";

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.prefs_appearance));
        SettingsUtils.addPreferencesFromResource(this, R.xml.appearance_preferences);
        boolean isTablet =
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                        ContextUtils.getApplicationContext());
        if (isTablet) {
            removePreferenceIfPresent(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        }

        if (!NightModeUtils.isNightModeSupported()) {
            removePreferenceIfPresent(PREF_UI_THEME);
        }

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker == null || !mBraveRewardsNativeWorker.isSupported()) {
            removePreferenceIfPresent(PREF_SHOW_BRAVE_REWARDS_ICON);
        }

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SPEEDREADER)) {
            removePreferenceIfPresent(PREF_BRAVE_ENABLE_SPEEDREADER);
        }
        if (!new BraveMultiWindowUtils().shouldShowEnableWindow(getActivity())) {
            removePreferenceIfPresent(PREF_ENABLE_MULTI_WINDOWS);
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) getPreferenceScreen().removePreference(preference);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        ChromeSwitchPreference showBraveRewardsIconPref =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_BRAVE_REWARDS_ICON);
        if (showBraveRewardsIconPref != null) {
            showBraveRewardsIconPref.setChecked(NtpUtil.shouldShowRewardsIcon());
            showBraveRewardsIconPref.setOnPreferenceChangeListener(this);
        }

        ChromeSwitchPreference adsSwitchPref =
                (ChromeSwitchPreference) findPreference(PREF_ADS_SWITCH);
        if (adsSwitchPref != null) {
            adsSwitchPref.setChecked(getPrefAdsInBackgroundEnabled());
            adsSwitchPref.setOnPreferenceChangeListener(this);
        }

        Preference nightModeEnabled = findPreference(PREF_BRAVE_NIGHT_MODE_ENABLED);
        nightModeEnabled.setOnPreferenceChangeListener(this);
        if (nightModeEnabled instanceof ChromeSwitchPreference) {
            ((ChromeSwitchPreference) nightModeEnabled)
                    .setChecked(ChromeFeatureList.isEnabled(
                            BraveFeatureList.FORCE_WEB_CONTENTS_DARK_MODE));
        }

        Preference enableBottomToolbar =
                findPreference(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        if (enableBottomToolbar != null) {
            enableBottomToolbar.setOnPreferenceChangeListener(this);
            if (enableBottomToolbar instanceof ChromeSwitchPreference) {
                boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                        ContextUtils.getApplicationContext());
                ((ChromeSwitchPreference) enableBottomToolbar)
                        .setChecked(
                                !isTablet && BottomToolbarConfiguration.isBottomToolbarEnabled());
            }
        }

        Preference disableSharingHub = findPreference(PREF_BRAVE_DISABLE_SHARING_HUB);
        if (disableSharingHub != null) {
            disableSharingHub.setOnPreferenceChangeListener(this);
            if (disableSharingHub instanceof ChromeSwitchPreference) {
                ((ChromeSwitchPreference) disableSharingHub)
                        .setChecked(
                                ChromeSharedPreferences.getInstance()
                                        .readBoolean(
                                                BravePreferenceKeys.BRAVE_DISABLE_SHARING_HUB,
                                                false));
            }
        }

        Preference enableTabGroups = findPreference(PREF_BRAVE_ENABLE_TAB_GROUPS);
        if (enableTabGroups != null) {
            enableTabGroups.setOnPreferenceChangeListener(this);
            if (enableTabGroups instanceof ChromeSwitchPreference) {
                ((ChromeSwitchPreference) enableTabGroups)
                        .setChecked(BraveTabUiFeatureUtilities.isBraveTabGroupsEnabled());
            }
        }

        Preference enableSpeedreader = findPreference(PREF_BRAVE_ENABLE_SPEEDREADER);
        if (enableSpeedreader != null) {
            enableSpeedreader.setOnPreferenceChangeListener(this);
            if (enableSpeedreader instanceof ChromeSwitchPreference) {
                ((ChromeSwitchPreference) enableSpeedreader)
                        .setChecked(
                                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                                        .getBoolean(BravePref.SPEEDREADER_PREF_ENABLED));
            }
        }

        Preference enableMultiWindow = findPreference(PREF_ENABLE_MULTI_WINDOWS);
        if (enableMultiWindow != null) {
            enableMultiWindow.setOnPreferenceChangeListener(this);
            if (enableMultiWindow instanceof ChromeSwitchPreference) {
                ((ChromeSwitchPreference) enableMultiWindow)
                        .setChecked(BraveMultiWindowUtils.shouldEnableMultiWindows());
            }
        }

        ChromeSwitchPreference showUndoButtonOnTabClosed =
                (ChromeSwitchPreference) findPreference(PREF_SHOW_UNDO_WHEN_TABS_CLOSED);
        if (showUndoButtonOnTabClosed != null) {
            showUndoButtonOnTabClosed.setOnPreferenceChangeListener(this);
            ((ChromeSwitchPreference) showUndoButtonOnTabClosed)
                    .setChecked(
                            ChromeSharedPreferences.getInstance()
                                    .readBoolean(
                                            BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, true));
        }
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public void onStart() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.addObserver(this);
        }
        super.onStart();
    }

    @Override
    public void onStop() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.removeObserver(this);
        }
        super.onStop();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        boolean shouldRelaunch = false;
        if (BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY.equals(key)) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Boolean originalStatus = BottomToolbarConfiguration.isBottomToolbarEnabled();
            prefs.edit()
                    .putBoolean(
                            BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, !originalStatus)
                    .apply();
            shouldRelaunch = true;
        } else if (PREF_SHOW_BRAVE_REWARDS_ICON.equals(key)) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(PREF_SHOW_BRAVE_REWARDS_ICON, !(boolean) newValue);
            sharedPreferencesEditor.apply();
            shouldRelaunch = true;
        } else if (PREF_ADS_SWITCH.equals(key)) {
            setPrefAdsInBackgroundEnabled((boolean) newValue);
        } else if (PREF_BRAVE_NIGHT_MODE_ENABLED.equals(key)) {
            BraveFeatureUtil.enableFeature(
                    BraveFeatureList.ENABLE_FORCE_DARK, (boolean) newValue, true);
            shouldRelaunch = true;
        } else if (PREF_BRAVE_DISABLE_SHARING_HUB.equals(key)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(
                            BravePreferenceKeys.BRAVE_DISABLE_SHARING_HUB, (boolean) newValue);
        } else if (PREF_BRAVE_ENABLE_TAB_GROUPS.equals(key)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, (boolean) newValue);
        } else if (PREF_BRAVE_ENABLE_SPEEDREADER.equals(key)) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.SPEEDREADER_PREF_ENABLED, (boolean) newValue);
            shouldRelaunch = true;
        } else if (PREF_ENABLE_MULTI_WINDOWS.equals(key)) {
            if (!(boolean) newValue) {
                if (MultiWindowUtils.getInstanceCount() > 1) {
                    BraveMultiWindowDialogFragment dialogFragment =
                            BraveMultiWindowDialogFragment.newInstance();
                    BraveMultiWindowDialogFragment.DismissListener dismissListener =
                            new BraveMultiWindowDialogFragment.DismissListener() {
                                @Override
                                public void onDismiss() {
                                    if (MultiWindowUtils.getInstanceCount() == 1) {
                                        if (preference instanceof ChromeSwitchPreference) {
                                            ((ChromeSwitchPreference) preference).setChecked(false);
                                            BraveMultiWindowUtils.updateEnableMultiWindows(false);
                                        }
                                    }
                                }
                            };
                    dialogFragment.setDismissListener(dismissListener);

                    dialogFragment.show(
                            getActivity().getSupportFragmentManager(),
                            "BraveMultiWindowDialogFragment");

                    return false;
                }
            }
            BraveMultiWindowUtils.updateEnableMultiWindows((boolean) newValue);
        } else if (PREF_SHOW_UNDO_WHEN_TABS_CLOSED.equals(key)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(
                            BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, (boolean) newValue);
        }
        if (shouldRelaunch) {
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }

        return true;
    }

    /**
     * Returns the user preference for whether the brave ads in background is enabled.
     *
     */
    public static boolean getPrefAdsInBackgroundEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_ADS_SWITCH, false);
    }

    /**
     * Sets the user preference for whether the brave ads in background is enabled.
     */
    public void setPrefAdsInBackgroundEnabled(boolean enabled) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ADS_SWITCH, enabled);
        sharedPreferencesEditor.apply();
    }
}
