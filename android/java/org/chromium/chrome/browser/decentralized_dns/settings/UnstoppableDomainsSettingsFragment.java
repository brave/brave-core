/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.decentralized_dns.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class UnstoppableDomainsSettingsFragment extends PreferenceFragmentCompat {
    static final String PREF_UNSTOPPABLE_DOMAINS_RESOLVE_METHOD =
            "unstoppable_domains_resolve_method";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.unstoppable_domains_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.unstoppable_domains_preferences);

        RadioButtonGroupDDnsResolveMethodPreference radioButtonGroupDDnsResolveMethodPreference =
                (RadioButtonGroupDDnsResolveMethodPreference) findPreference(
                        PREF_UNSTOPPABLE_DOMAINS_RESOLVE_METHOD);

        radioButtonGroupDDnsResolveMethodPreference.initialize(
                BraveLocalState.get().getInteger(BravePref.UNSTOPPABLE_DOMAINS_RESOLVE_METHOD));

        radioButtonGroupDDnsResolveMethodPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    int method = (int) newValue;
                    BraveLocalState.get().setInteger(
                            BravePref.UNSTOPPABLE_DOMAINS_RESOLVE_METHOD, method);
                    return true;
                });
    }
}
