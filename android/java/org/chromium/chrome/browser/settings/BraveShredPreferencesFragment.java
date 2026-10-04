/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Build;
import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.ui.UiUtils;

/** Fragment to manage Shred settings. */
@NullMarked
public class BraveShredPreferencesFragment extends BravePreferenceFragment {
    private static final String PREF_AUTO_SHRED_STORAGE = "auto_shred_storage";
    private static final String PREF_AUTO_SHRED_REMOVE_HISTORY = "shred_removes_history_switch";
    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();
    private ChromeSwitchPreference mRemoveHistorySwitch;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_shred_preferences);
        mPageTitle.set(getString(R.string.brave_shields_shred_settings_text));

        BraveShredPreference autoShredPreference =
                (BraveShredPreference) findPreference(PREF_AUTO_SHRED_STORAGE);
        autoShredPreference.initialize(BraveShieldsContentSettings.getAutoShredPref());

        autoShredPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    BraveShieldsContentSettings.setAutoShredPref(newValue.toString());
                    return true;
                });

        mRemoveHistorySwitch =
                (ChromeSwitchPreference) findPreference(PREF_AUTO_SHRED_REMOVE_HISTORY);
        mRemoveHistorySwitch.setChecked(
                BraveShieldsContentSettings.isShredBrowsingHistoryEnabled(getProfile()));
        mRemoveHistorySwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    BraveShieldsContentSettings.setShredBrowsingHistoryEnabled(
                            (boolean) newValue, getProfile());
                    return true;
                });
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    // This fragment displays a custom radio-button widget with no static titled preferences.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveShredPreferencesFragment.class.getName(),
                    BaseSearchIndexProvider.INDEX_OPT_OUT);

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.O_MR1) {
            UiUtils.setNavigationBarIconColor(
                    getActivity().getWindow().getDecorView(),
                    getResources().getBoolean(R.bool.window_light_navigation_bar));
        }

        setDivider(null);
    }
}
