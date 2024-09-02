/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
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

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String s) {
        // These strings are not used in Brave, but we get them from automated string translation
        // process. These checks are just making sure that strings are still used in Chromium to
        // avoid lint issues.
        assert R.string.chrome_additional_terms_of_service_title > 0
                : "Something has changed in the upstream!";
        assert R.string.google_privacy_policy_url > 0 : "Something has changed in the upstream!";
        assert R.string.sync_reading_list > 0 : "Something has changed in the upstream!";

        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_license_preferences);
        mPageTitle.set(getString(R.string.brave_license_text));
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

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }
}
