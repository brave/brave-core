/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.appearance.settings.AppearanceSettingsFragment;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowDialogFragment;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowUtils;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager.PersistedInstanceType;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.toolbar.ToolbarPositionController;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.settings.AddressBarSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class AppearancePreferences extends AppearanceSettingsFragment
        implements Preference.OnPreferenceChangeListener, BraveRewardsObserver {
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON = "hide_brave_rewards_icon";
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON_MIGRATION =
            "hide_brave_rewards_icon_migration";
    public static final String PREF_SHOW_BRAVE_REWARDS_ICON = "show_brave_rewards_icon";
    public static final String PREF_ADS_SWITCH = "ads_switch";
    public static final String PREF_BRAVE_NIGHT_MODE_ENABLED = "brave_night_mode_enabled_key";
    public static final String PREF_BRAVE_DISABLE_SHARING_HUB = "brave_disable_sharing_hub";
    public static final String PREF_BRAVE_ENABLE_TAB_GROUPS = "brave_enable_tab_groups";
    public static final String PREF_ENABLE_MULTI_WINDOWS = "enable_multi_windows";
    public static final String PREF_SHOW_UNDO_WHEN_TABS_CLOSED = "show_undo_when_tabs_closed";
    public static final String PREF_ADDRESS_BAR = "address_bar";
    private static final String PREF_BRAVE_CUSTOMIZE_MENU = "brave_customize_menu";

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_appearance_preferences);

        // Forward the custom menu item keys from appearance to custom menu item preference screen.
        CustomizeBraveMenu.propagateMenuItemExtras(
                findPreference(PREF_BRAVE_CUSTOMIZE_MENU), getArguments());

        boolean isTablet =
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                        ContextUtils.getApplicationContext());
        if (isTablet) {
            removePreferenceIfPresent(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        }

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker == null || !mBraveRewardsNativeWorker.isSupported()) {
            removePreferenceIfPresent(PREF_SHOW_BRAVE_REWARDS_ICON);
        }

        if (!new BraveMultiWindowUtils().shouldShowEnableWindow(getActivity())) {
            removePreferenceIfPresent(PREF_ENABLE_MULTI_WINDOWS);
        }

        if (!ToolbarPositionController.isToolbarPositionCustomizationEnabled(getContext(), false)) {
            removePreferenceIfPresent(PREF_ADDRESS_BAR);
        }

        // Correct the order of the preferences.
        setPreferenceOrder(AppearanceSettingsFragment.PREF_UI_THEME, 0);
        setPreferenceOrder(PREF_BRAVE_CUSTOMIZE_MENU, 1);
        setPreferenceOrder(PREF_ADDRESS_BAR, 2);
    }

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
    public void onStart() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.addObserver(this);
        }
        super.onStart();

        if (ToolbarPositionController.isToolbarPositionCustomizationEnabled(getContext(), false)) {
            updatePreferenceTitle(
                    PREF_ADDRESS_BAR, AddressBarSettingsFragment.getTitle(getContext()));
            updatePreferenceIcon(
                    PREF_ADDRESS_BAR,
                    BottomToolbarConfiguration.isToolbarTopAnchored()
                            ? R.drawable.ic_browser_mobile_tabs_top
                            : R.drawable.ic_browser_mobile_tabs_bottom);
        }

        Preference enableBottomToolbar =
                findPreference(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        if (enableBottomToolbar instanceof ChromeSwitchPreference) {
            if (BottomToolbarConfiguration.isToolbarTopAnchored()) {
                boolean isTablet =
                        DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                                ContextUtils.getApplicationContext());
                ((ChromeSwitchPreference) enableBottomToolbar)
                        .setChecked(
                                !isTablet
                                        && BottomToolbarConfiguration
                                                .isBraveBottomControlsEnabled());
            }
            if (BottomToolbarConfiguration.isToolbarBottomAnchored()) {
                updatePreferenceSummary(
                        BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                        R.string.brave_bottom_navigation_toolbar_disabled_summary);
            } else {
                updatePreferenceSummary(
                        BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                        ((ChromeSwitchPreference) enableBottomToolbar).isChecked()
                                ? R.string.text_on
                                : R.string.text_off);
            }
            ((ChromeSwitchPreference) enableBottomToolbar)
                    .setEnabled(BottomToolbarConfiguration.isToolbarTopAnchored());
        }

        updatePreferenceIcon(
                AppearanceSettingsFragment.PREF_TOOLBAR_SHORTCUT,
                R.drawable.ic_browser_customizable_shortcut);

        // Update the UI theme preference icon and clear the summary.
        updatePreferenceIcon(AppearanceSettingsFragment.PREF_UI_THEME, R.drawable.ic_theme_system);
        clearPreferenceSummary(AppearanceSettingsFragment.PREF_UI_THEME);
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
            Boolean originalStatus = BottomToolbarConfiguration.isBraveBottomControlsEnabled();
            updatePreferenceSummary(
                    BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                    !originalStatus ? R.string.text_on : R.string.text_off);
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(
                            BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, !originalStatus);
            shouldRelaunch = true;
        } else if (PREF_SHOW_BRAVE_REWARDS_ICON.equals(key)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(PREF_SHOW_BRAVE_REWARDS_ICON, !(boolean) newValue);
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
        } else if (PREF_ENABLE_MULTI_WINDOWS.equals(key)) {
            if (!(boolean) newValue) {
                if (MultiWindowUtils.getInstanceCountWithFallback(PersistedInstanceType.ACTIVE)
                        > 1) {
                    BraveMultiWindowDialogFragment dialogFragment =
                            BraveMultiWindowDialogFragment.newInstance();
                    BraveMultiWindowDialogFragment.DismissListener dismissListener =
                            new BraveMultiWindowDialogFragment.DismissListener() {
                                @Override
                                public void onDismiss() {
                                    if (MultiWindowUtils.getInstanceCountWithFallback(
                                                    PersistedInstanceType.ACTIVE)
                                            == 1) {
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

    /** Returns the user preference for whether the brave ads in background is enabled. */
    public static boolean getPrefAdsInBackgroundEnabled() {
        return ChromeSharedPreferences.getInstance().readBoolean(PREF_ADS_SWITCH, false);
    }

    /** Sets the user preference for whether the brave ads in background is enabled. */
    public void setPrefAdsInBackgroundEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance().writeBoolean(PREF_ADS_SWITCH, enabled);
    }

    private void updatePreferenceIcon(String preferenceString, int drawable) {
        Preference preference = findPreference(preferenceString);
        if (preference != null) {
            preference.setIcon(drawable);
        }
    }

    private void updatePreferenceTitle(String preferenceString, CharSequence title) {
        Preference preference = findPreference(preferenceString);
        if (preference != null) {
            preference.setTitle(title);
        }
    }

    private void updatePreferenceSummary(String preferenceString, int summaryId) {
        Preference preference = findPreference(preferenceString);
        if (preference != null) {
            preference.setSummary(summaryId);
        }
    }

    private void clearPreferenceSummary(String preferenceString) {
        Preference preference = findPreference(preferenceString);
        if (preference != null) {
            preference.setSummary("");
        }
    }

    private void setPreferenceOrder(String key, int order) {
        Preference preference = findPreference(key);
        if (preference != null) {
            preference.setOrder(order);
        }
    }
}
