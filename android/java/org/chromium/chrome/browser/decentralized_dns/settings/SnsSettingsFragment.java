/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.decentralized_dns.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class SnsSettingsFragment extends PreferenceFragmentCompat {
    static final String PREF_SNS_RESOLVE_METHOD = "sns_resolve_method";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.sns_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.sns_preferences);

        RadioButtonGroupDDnsResolveMethodPreference radioButtonGroupDDnsResolveMethodPreference =
                (RadioButtonGroupDDnsResolveMethodPreference) findPreference(
                        PREF_SNS_RESOLVE_METHOD);

        radioButtonGroupDDnsResolveMethodPreference.initialize(
                BravePrefServiceBridge.getInstance().getSnsResolveMethod());

        radioButtonGroupDDnsResolveMethodPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    int method = (int) newValue;
                    BravePrefServiceBridge.getInstance().setSnsResolveMethod(method);
                    return true;
                });
    }
}
