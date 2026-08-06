/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.decentralized_dns.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.SettingsFragment.AnimationType;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;

public class SnsSettingsFragment extends ChromeBaseSettingsFragment {
    static final String PREF_SNS_RESOLVE_METHOD = "sns_resolve_method";

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        mPageTitle.set(getString(R.string.sns_title));
        SettingsUtils.addPreferencesFromResource(this, R.xml.sns_preferences);

        RadioButtonGroupDDnsResolveMethodPreference radioButtonGroupDDnsResolveMethodPreference =
                (RadioButtonGroupDDnsResolveMethodPreference) findPreference(
                        PREF_SNS_RESOLVE_METHOD);

        radioButtonGroupDDnsResolveMethodPreference.initialize(
                BraveLocalState.get().getInteger(BravePref.SNS_RESOLVE_METHOD));

        radioButtonGroupDDnsResolveMethodPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    int method = (int) newValue;
                    BraveLocalState.get().setInteger(BravePref.SNS_RESOLVE_METHOD, method);
                    return true;
                });
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }

    // The resolve-method screen is a custom radio-button widget with no static titled preferences.
    // The entry itself is indexed from the parent Brave Shields & privacy screen, so opt out here.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    SnsSettingsFragment.class.getName(), BaseSearchIndexProvider.INDEX_OPT_OUT);
}
