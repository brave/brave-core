/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;

import com.google.android.material.snackbar.Snackbar;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave.browser.brave_origin.BraveOriginServiceFactory;
import org.chromium.brave_origin.mojom.BraveOriginSettingsHandler;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.brave_origin.BraveOriginSubscriptionPrefs;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.policy.BravePolicyConstants;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

import java.util.Map;

/** Fragment for Brave Origin purchase preferences. */
@NullMarked
public class BraveOriginPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "BraveOriginPrefs";

    // Preference keys
    private static final String PREF_REWARDS_SWITCH = "rewards_switch";
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
    private static final String PREF_LINK_PURCHASE = "link_purchase";

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();
    @Nullable private BraveOriginSettingsHandler mBraveOriginSettingsHandler;
    @Nullable private Snackbar mRestartSnackbar;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_origin_preferences);
        mPageTitle.set(getString(R.string.menu_origin));

        // Initialize BraveOriginSettingsHandler
        Profile profile = getProfile();
        if (profile != null) {
            mBraveOriginSettingsHandler =
                    BraveOriginServiceFactory.getInstance()
                            .getBraveOriginSettingsHandler(profile, null);
        }

        // Set up toggle preferences
        setupTogglePreference(PREF_REWARDS_SWITCH);
        setupTogglePreference(PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH);
        if (ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)) {
            setupTogglePreference(PREF_EMAIL_ALIASES_SWITCH);
        } else {
            ChromeSwitchPreference emailAliasesPref =
                    (ChromeSwitchPreference) findPreference(PREF_EMAIL_ALIASES_SWITCH);
            if (emailAliasesPref != null) {
                emailAliasesPref.setVisible(false);
            }
        }
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
                        showResetConfirmationDialog();
                        return true;
                    });
        }

        Preference linkPurchase = findPreference(PREF_LINK_PURCHASE);
        if (linkPurchase != null) {
            linkPurchase.setOnPreferenceClickListener(
                    preference -> {
                        TabUtils.openURLWithBraveActivity(
                                LinkSubscriptionUtils.getBraveAccountLinkUrl(
                                        InAppPurchaseWrapper.SubscriptionProduct.ORIGIN));
                        return true;
                    });
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        boolean isEnabled = (Boolean) newValue;

        updateToggleDescription(preference, isEnabled);
        String policyKey = getPolicyKeyForPreference(key);
        if (policyKey == null || mBraveOriginSettingsHandler == null) {
            return false;
        }
        // For DISABLED policies, invert the value (checked = enabled = false policy value)
        // For ENABLED policies, use the value as-is (checked = enabled = true policy value)
        boolean policyValue =
                BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey) ? !isEnabled : isEnabled;
        mBraveOriginSettingsHandler.setPolicyValue(
                policyKey,
                policyValue,
                (success) -> {
                    if (!success) {
                        Log.e(TAG, "Failed to set policy value for " + policyKey);
                    } else {
                        showRestartSnackbar();
                    }
                });
        return true;
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    /**
     * Shows a custom snackbar prompting the user to restart the browser after toggling a feature.
     */
    private void showRestartSnackbar() {
        // Don't show another snackbar if one is already visible
        if (mRestartSnackbar != null && mRestartSnackbar.isShown()) {
            return;
        }

        View view = getView();
        if (view == null) {
            return;
        }

        mRestartSnackbar = Snackbar.make(view, "", Snackbar.LENGTH_INDEFINITE);
        Snackbar.SnackbarLayout snackbarLayout =
                (Snackbar.SnackbarLayout) mRestartSnackbar.getView();

        // Remove default snackbar content, padding, and background so custom layout shows through
        snackbarLayout.removeAllViews();
        snackbarLayout.setPadding(0, 0, 0, 0);
        snackbarLayout.setBackground(null);

        // Inflate custom layout
        View customView =
                LayoutInflater.from(requireContext())
                        .inflate(R.layout.origin_restart_snackbar, null);

        // Set up restart action
        TextView actionButton = customView.findViewById(R.id.snackbar_action);
        actionButton.setOnClickListener(
                v -> {
                    dismissRestartSnackbar();
                    BraveRelaunchUtils.restart();
                });

        // Set up close button
        ImageButton closeButton = customView.findViewById(R.id.snackbar_close);
        closeButton.setOnClickListener(v -> dismissRestartSnackbar());

        snackbarLayout.addView(customView, 0);
        mRestartSnackbar.show();
    }

    private void dismissRestartSnackbar() {
        if (mRestartSnackbar != null && mRestartSnackbar.isShown()) {
            mRestartSnackbar.dismiss();
        }
        mRestartSnackbar = null;
    }

    /**
     * Sets up a toggle preference with listener and initial state. Also initializes the preference
     * value from the policy service if available.
     *
     * @param key The preference key
     */
    private void setupTogglePreference(String key) {
        ChromeSwitchPreference preference = (ChromeSwitchPreference) findPreference(key);
        if (preference == null) {
            assert false : "Preference not found for key: " + key;
            return;
        }
        preference.setOnPreferenceChangeListener(this);
        updateToggleDescription(preference, preference.isChecked());

        // Initialize from policy service if available
        String policyKey = getPolicyKeyForPreference(key);
        if (policyKey == null || mBraveOriginSettingsHandler == null) {
            return;
        }
        mBraveOriginSettingsHandler.getPolicyValue(
                policyKey,
                (value) -> {
                    if (value != null) {
                        // For DISABLED policies, invert the value (!value)
                        // For ENABLED policies, use the value as-is
                        boolean checkedValue =
                                BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey)
                                        ? !value
                                        : value;
                        preference.setChecked(checkedValue);
                        updateToggleDescription(preference, checkedValue);
                    }
                });
    }

    /**
     * Gets the policy key for a given preference key.
     *
     * @param preferenceKey The preference key
     * @return The policy key, or null if not mapped
     */
    @Nullable
    private String getPolicyKeyForPreference(String preferenceKey) {
        // Map preference keys to policy keys
        if (PREF_REWARDS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_REWARDS_DISABLED;
        } else if (PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_P3A_ENABLED;
        } else if (PREF_LEO_AI_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_AI_CHAT_ENABLED;
        } else if (PREF_NEWS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_NEWS_DISABLED;
        } else if (PREF_STATISTICS_REPORTING_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_STATS_PING_ENABLED;
        } else if (PREF_VPN_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_VPN_DISABLED;
        } else if (PREF_WALLET_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_WALLET_DISABLED;
        } else if (PREF_WEB_DISCOVERY_PROJECT_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_WEB_DISCOVERY_ENABLED;
        }
        // TODO: Add mappings for other preferences as they are implemented
        // PREF_EMAIL_ALIASES_SWITCH - no policy mapping found
        return null;
    }

    private void showResetConfirmationDialog() {
        View dialogView =
                LayoutInflater.from(requireContext()).inflate(R.layout.origin_reset_dialog, null);

        AlertDialog dialog =
                new AlertDialog.Builder(
                                requireContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog)
                        .setView(dialogView)
                        .create();

        dialogView.findViewById(R.id.reset_dialog_cancel).setOnClickListener(v -> dialog.dismiss());
        dialogView
                .findViewById(R.id.reset_dialog_confirm)
                .setOnClickListener(
                        v -> {
                            dialog.dismiss();
                            resetAllToggles();
                        });

        dialog.show();
    }

    private void resetAllToggles() {
        String[] toggleKeys = {
            PREF_REWARDS_SWITCH,
            PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH,
            PREF_EMAIL_ALIASES_SWITCH,
            PREF_LEO_AI_SWITCH,
            PREF_NEWS_SWITCH,
            PREF_STATISTICS_REPORTING_SWITCH,
            PREF_VPN_SWITCH,
            PREF_WALLET_SWITCH,
            PREF_WEB_DISCOVERY_PROJECT_SWITCH,
        };

        boolean anyChanged = false;
        for (String key : toggleKeys) {
            ChromeSwitchPreference pref = (ChromeSwitchPreference) findPreference(key);
            if (pref == null || !pref.isChecked()) {
                continue;
            }
            pref.setChecked(false);
            updateToggleDescription(pref, false);
            anyChanged = true;

            String policyKey = getPolicyKeyForPreference(key);
            if (policyKey != null && mBraveOriginSettingsHandler != null) {
                boolean policyValue = BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey);
                mBraveOriginSettingsHandler.setPolicyValue(
                        policyKey,
                        policyValue,
                        (success) -> {
                            if (!success) {
                                Log.e(TAG, "Failed to reset policy value for " + policyKey);
                            }
                        });
            }
        }

        if (anyChanged) {
            showRestartSnackbar();
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

    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveOriginPreferences.class.getName(), R.xml.brave_origin_preferences) {

                @Override
                public void initPreferenceXml(
                        Context context,
                        SettingsIndexData indexData,
                        Map<String, SearchIndexProvider> providerMap) {
                    super.initPreferenceXml(context, indexData, providerMap);
                    // brave_main_preferences.xml is not processed by
                    // MainSettings.SEARCH_INDEX_DATA_PROVIDER, so we add the brave_origin
                    // entry and child-parent link manually so resolveIndex() does not treat
                    // BraveOriginPreferences entries as orphans.
                    indexData.addEntryForKey(
                            "org.chromium.chrome.browser.settings.MainSettings",
                            "brave_origin",
                            R.string.menu_origin,
                            /* summaryId= */ 0,
                            BraveOriginPreferences.class.getName());
                }

                @Override
                public void updateDynamicPreferences(Context context, SettingsIndexData indexData) {
                    String frag = BraveOriginPreferences.class.getName();
                    if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ORIGIN)) {
                        indexData.removeEntryForKey(
                                "org.chromium.chrome.browser.settings.MainSettings",
                                "brave_origin");
                        return;
                    }
                    // origin_description is an informational widget with no title; exclude it.
                    indexData.removeEntryForKey(frag, "origin_description");
                    if (!ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)) {
                        indexData.removeEntryForKey(frag, PREF_EMAIL_ALIASES_SWITCH);
                    }
                }
            };

    @Override
    public void onDestroy() {
        dismissRestartSnackbar();
        super.onDestroy();
        if (mBraveOriginSettingsHandler != null) {
            mBraveOriginSettingsHandler.close();
            mBraveOriginSettingsHandler = null;
        }
    }
}
