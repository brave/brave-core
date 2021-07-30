/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Browser;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import androidx.annotation.Nullable;
import androidx.vectordrawable.graphics.drawable.VectorDrawableCompat;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.UiUtils;

/**
 * Fragment to manage webrtc policy settings.
 */
public class BraveWebrtcPolicyPreferencesFragment extends BravePreferenceFragment {
    static final String PREF_WEBRTC_POLICY = "webrtc_policy";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_webrtc_policy_preferences);
        getActivity().setTitle(R.string.settings_webrtc_policy_label);

        BraveWebrtcPolicyPreference webrtcPolicyPreference =
                (BraveWebrtcPolicyPreference) findPreference(PREF_WEBRTC_POLICY);
        webrtcPolicyPreference.initialize(BravePrefServiceBridge.getInstance().getWebrtcPolicy());

        webrtcPolicyPreference.setOnPreferenceChangeListener((preference, newValue) -> {
            BravePrefServiceBridge.getInstance().setWebrtcPolicy((int) newValue);
            return true;
        });
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.O_MR1) {
            UiUtils.setNavigationBarIconColor(getActivity().getWindow().getDecorView(),
                    getResources().getBoolean(R.bool.window_light_navigation_bar));
        }

        setDivider(null);
    }
}
