/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.SettingsUtils;

import java.io.IOException;
import java.io.InputStream;
import java.util.Scanner;

/**
 * Fragment to display Brave license information.
 */
public class BraveLicensePreferences extends BravePreferenceFragment {
    private static final String TAG = "BraveLicense";

    private static final String PREF_BRAVE_LICENSE_TEXT = "brave_license_text";
    private static final String ASSET_BRAVE_LICENSE = "LICENSE.html";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String s) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_license_preferences);
        getActivity().setTitle(R.string.brave_license_text);
        BraveLicensePreference licenseText =
                (BraveLicensePreference) findPreference(PREF_BRAVE_LICENSE_TEXT);
        try {
            InputStream in = getActivity().getAssets().open(ASSET_BRAVE_LICENSE);
            Scanner scanner = new Scanner(in).useDelimiter("\\A");
            String summary = scanner.hasNext() ? scanner.next() : "";
            in.close();
            licenseText.setSummary(BraveRewardsHelper.spannedFromHtmlString(summary));
        } catch (IOException e) {
            Log.e(TAG, "Could not load license text: " + e);
        }
    }
}
