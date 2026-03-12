/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.os.Bundle;

import androidx.preference.Preference;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;

public class BraveWalletSolanaConnectedSites extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String PREF_BRAVE_WALLET_SOLANA_CONNECTED_SITES =
            "pref_brave_wallet_solana_connected_sites";
    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mPageTitle.set(getString(R.string.settings_solana_title));
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_solana_preferences);
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }

    // This fragment displays a dynamic connected-sites widget; there are no static preferences
    // to index.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveWalletSolanaConnectedSites.class.getName(),
                    BaseSearchIndexProvider.INDEX_OPT_OUT);

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        BraveWalletSolanaConnectedSitesPreference pref =
                (BraveWalletSolanaConnectedSitesPreference) findPreference(
                        PREF_BRAVE_WALLET_SOLANA_CONNECTED_SITES);
        pref.destroy();
    }
}
