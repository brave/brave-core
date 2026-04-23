/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.base.Log;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;

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

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @SuppressWarnings("ScannerUseDelimiter")
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
        try (BufferedReader reader = new BufferedReader(
                 new InputStreamReader(
                         getActivity().getAssets().open(ASSET_BRAVE_LICENSE),
                         StandardCharsets.UTF_8))) {
        String summary = reader.lines().collect(Collectors.joining("\n"));
        licenseText.setSummary(BraveRewardsHelper.spannedFromHtmlString(summary));
        } catch (IOException e) {
            Log.e(TAG, "Could not load license text: " + e);
        }
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    // The screen only displays a dynamically-loaded HTML license text; there are no static
    // settings to index.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveLicensePreferences.class.getName(), BaseSearchIndexProvider.INDEX_OPT_OUT);
}
