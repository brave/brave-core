/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.appcompat.content.res.AppCompatResources;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

/** Fragment for Brave Origin subscription preferences. */
@NullMarked
public class BraveOriginPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "BraveOriginPreferences";

    // Preference keys
    private static final String PREF_REWARDS_SWITCH = "rewards_switch";
    private static final String PREF_CRASH_REPORTS_SWITCH = "crash_reports_switch";
    private static final String PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH =
            "privacy_preserving_analytics_switch";
    private static final String PREF_EMAIL_ALIASES_SWITCH = "email_aliases_switch";
    private static final String PREF_LEO_AI_SWITCH = "leo_ai_switch";
    private static final String PREF_NEWS_SWITCH = "news_switch";
    private static final String PREF_STATISTICS_REPORTING_SWITCH = "statistics_reporting_switch";
    private static final String PREF_VPN_SWITCH = "vpn_switch";
    private static final String PREF_WALLET_SWITCH = "wallet_switch";
    private static final String PREF_WEB_DISCOVERY_PROJECT_SWITCH = "web_discovery_project_switch";
    private static final String PREF_RESET_TO_DEFAULTS = "reset_to_defaults";
    // TODO: Uncomment when subscription status is implemented
    // private static final String PREF_SUBSCRIPTION_STATUS = "subscription_status";
    // TODO: Uncomment when subscription expires is implemented
    // private static final String PREF_SUBSCRIPTION_EXPIRES = "subscription_expires";
    // TODO: Uncomment when subscription manage is implemented
    private static final String PREF_SUBSCRIPTION_MANAGE = "subscription_manage";
    private static final String PREF_LINK_SUBSCRIPTION = "link_subscription";

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_origin_preferences);
        mPageTitle.set(getString(R.string.menu_origin));

        // Set up toggle preferences
        setupTogglePreference(PREF_REWARDS_SWITCH);
        setupTogglePreference(PREF_CRASH_REPORTS_SWITCH);
        setupTogglePreference(PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH);
        setupTogglePreference(PREF_EMAIL_ALIASES_SWITCH);
        setupTogglePreference(PREF_LEO_AI_SWITCH);
        setupTogglePreference(PREF_NEWS_SWITCH);
        setupTogglePreference(PREF_STATISTICS_REPORTING_SWITCH);
        setupTogglePreference(PREF_VPN_SWITCH);
        setupTogglePreference(PREF_WALLET_SWITCH);
        setupTogglePreference(PREF_WEB_DISCOVERY_PROJECT_SWITCH);

        // Set up click preferences
        Preference resetToDefaults = findPreference(PREF_RESET_TO_DEFAULTS);
        if (resetToDefaults != null) {
            resetToDefaults.setOnPreferenceClickListener(
                    preference -> {
                        // TODO: Implement reset to defaults functionality
                        return true;
                    });
        }

        Preference subscriptionManage = findPreference(PREF_SUBSCRIPTION_MANAGE);
        if (subscriptionManage != null) {
            subscriptionManage.setOnPreferenceClickListener(
                    preference -> {
                        // TODO: Open subscription management
                        return true;
                    });
        }

        Preference linkSubscription = findPreference(PREF_LINK_SUBSCRIPTION);
        if (linkSubscription != null) {
            linkSubscription.setOnPreferenceClickListener(
                    preference -> {
                        // TODO: Open link subscription dialog
                        return true;
                    });
        }

        // Update subscription information
        updateSubscriptionInfo();

        // Apply tinting to all preference icons for proper dark theme support
        applyIconTinting(getPreferenceScreen(), null);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        boolean isEnabled = (Boolean) newValue;

        updateToggleDescription(preference, isEnabled);
        if (PREF_REWARDS_SWITCH.equals(key)) {
            // TODO: Handle rewards toggle change
            return true;
        } else if (PREF_CRASH_REPORTS_SWITCH.equals(key)) {
            // TODO: Handle crash reports toggle change
            return true;
        } else if (PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH.equals(key)) {
            // TODO: Handle privacy preserving analytics toggle change
            return true;
        } else if (PREF_EMAIL_ALIASES_SWITCH.equals(key)) {
            // TODO: Handle email aliases toggle change
            return true;
        } else if (PREF_LEO_AI_SWITCH.equals(key)) {
            // TODO: Handle Leo AI toggle change
            return true;
        } else if (PREF_NEWS_SWITCH.equals(key)) {
            // TODO: Handle news toggle change
            return true;
        } else if (PREF_STATISTICS_REPORTING_SWITCH.equals(key)) {
            // TODO: Handle statistics reporting toggle change
            return true;
        } else if (PREF_VPN_SWITCH.equals(key)) {
            // TODO: Handle VPN toggle change
            return true;
        } else if (PREF_WALLET_SWITCH.equals(key)) {
            // TODO: Handle wallet toggle change
            return true;
        } else if (PREF_WEB_DISCOVERY_PROJECT_SWITCH.equals(key)) {
            // Use the generated policy constant
            // String policyKey = BravePolicyConstants.BRAVE_WEB_DISCOVERY_ENABLED;
            // TODO: Call BraveOriginService to set the policy value
            // braveOriginService.setPolicyValue(policyKey, enabled);
            return true;
        }
        return false;
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void updateSubscriptionInfo() {
        // TODO: Fetch actual subscription data and update the status and expires preferences
    }

    /**
     * Sets up a toggle preference with listener and initial state.
     *
     * @param key The preference key
     */
    private void setupTogglePreference(String key) {
        ChromeSwitchPreference preference = (ChromeSwitchPreference) findPreference(key);
        if (preference != null) {
            preference.setOnPreferenceChangeListener(this);
            updateToggleDescription(preference, preference.isChecked());
        }
    }

    /**
     * Updates the description of a toggle preference based on its enabled state. Shows "This
     * feature is enabled because you opted into it" when enabled, removes the description when
     * disabled.
     *
     * @param preference The preference to update
     * @param isEnabled Whether the toggle is enabled
     */
    private void updateToggleDescription(Preference preference, boolean isEnabled) {
        if (isEnabled) {
            preference.setSummary(R.string.origin_toggle_enabled_description);
        } else {
            preference.setSummary(null);
        }
    }

    /**
     * Recursively applies tinting to all preferences with icons.
     *
     * @param preferenceGroup The preference group to process
     * @param tintList The color state list to apply (fetched once on first call)
     */
    private void applyIconTinting(
            PreferenceGroup preferenceGroup, @Nullable ColorStateList tintList) {
        // Fetch the tint list once on the first call
        if (tintList == null) {
            tintList =
                    AppCompatResources.getColorStateList(
                            requireContext(), R.color.default_icon_color_secondary_tint_list);
            if (tintList == null) {
                return;
            }
        }

        // Process all preferences in this group
        for (int i = 0; i < preferenceGroup.getPreferenceCount(); i++) {
            Preference preference = preferenceGroup.getPreference(i);

            // Apply tinting if this preference has an icon
            if (preference.getIcon() != null) {
                applyTintToPreferenceIcon(preference, tintList);
            }

            // Recursively process nested preference groups
            if (preference instanceof PreferenceGroup) {
                applyIconTinting((PreferenceGroup) preference, tintList);
            }
        }
    }

    /**
     * Applies the default icon tint to a preference's icon drawable. This ensures icons display
     * correctly in both light and dark themes by using the default secondary icon color.
     *
     * @param preference The preference whose icon should be tinted
     * @param tintList The color state list to apply to the icon
     */
    private void applyTintToPreferenceIcon(Preference preference, ColorStateList tintList) {
        Drawable icon = preference.getIcon();
        if (icon == null) {
            return;
        }

        // Mutate the drawable to avoid affecting other instances
        icon = icon.mutate();

        // Apply the tint list
        icon.setTintList(tintList);

        // Set the tinted icon back to the preference
        preference.setIcon(icon);
    }
}
