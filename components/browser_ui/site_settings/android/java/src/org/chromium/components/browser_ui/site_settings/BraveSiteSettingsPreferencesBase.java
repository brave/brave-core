/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.components.browser_ui.settings.SettingsUtils;

import java.util.HashMap;

public class BraveSiteSettingsPreferencesBase extends BaseSiteSettingsFragment {
    private static final String ADS_KEY = "ads";
    private static final String BACKGROUND_SYNC_KEY = "background_sync";
    private static final String IDLE_DETECTION = "idle_detection";
    private static final String DIVIDER_KEY = "divider";
    private static final String PERMISSION_AUTOREVOCATION_KEY = "permission_autorevocation";
    private static final String ETHEREUM_CONNECTED_SITES_KEY = "ethereum_connected_sites";
    private static final String SOLANA_CONNECTED_SITES_KEY = "solana_connected_sites";

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Add brave's additional preferences here because |onCreatePreference| is not called
        // by subclass (SiteSettingsPreferences::onCreatePreferences()).
        // But, calling here has same effect because |onCreatePreferences()| is called by onCreate().
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_site_settings_preferences);
        configureBravePreferences();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    @Override
    public void onResume() {
        super.onResume();
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public Preference findPreference(CharSequence key) {
        Preference result = super.findPreference(key);
        if (result == null) {
            result = mRemovedPreferences.get((String) key);
        }
        return result;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

    private void configureBravePreferences() {
        removePreferenceIfPresent(IDLE_DETECTION);
        removePreferenceIfPresent(ADS_KEY);
        removePreferenceIfPresent(BACKGROUND_SYNC_KEY);

        // Hide Ethereum and Solana connected sites when wallet is disabled by policy.
        if (isWalletDisabledByPolicy()) {
            removePreferenceIfPresent(ETHEREUM_CONNECTED_SITES_KEY);
            removePreferenceIfPresent(SOLANA_CONNECTED_SITES_KEY);
        }

        // We want to place these Settings at the bottom.
        // See https://github.com/brave/brave-browser/issues/46547
        // for the context
        Preference prefDivider = getPreferenceScreen().findPreference(DIVIDER_KEY);
        Preference prefPermissionAutorevocation =
                getPreferenceScreen().findPreference(PERMISSION_AUTOREVOCATION_KEY);
        assert prefDivider != null && prefPermissionAutorevocation != null
                : "Remove the order adjustment if the prefs are removed from upstream";
        if (prefDivider != null && prefPermissionAutorevocation != null) {
            Preference prefSolanaConnectedSites =
                    getPreferenceScreen().findPreference(SOLANA_CONNECTED_SITES_KEY);
            // Solana preference may be removed if wallet is disabled by policy.
            if (prefSolanaConnectedSites != null) {
                int solanaConnectedSitesOrder = prefSolanaConnectedSites.getOrder();
                prefDivider.setOrder(solanaConnectedSitesOrder + 1);
                prefPermissionAutorevocation.setOrder(solanaConnectedSitesOrder + 2);
            }
        }
    }

    private boolean isWalletDisabledByPolicy() {
        if (!hasSiteSettingsDelegate()) {
            return false;
        }
        SiteSettingsDelegate delegate = getSiteSettingsDelegate();
        if (delegate instanceof BraveWalletSiteSettingsDelegate) {
            return ((BraveWalletSiteSettingsDelegate) delegate).isWalletDisabledByPolicy();
        }
        return false;
    }
}
