/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.settings;

import android.content.Context;

import androidx.preference.Preference;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.chrome.browser.settings.search.ChromeBaseSearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

@NullMarked
public abstract class BraveAutofillPaymentMethodsFragmentBase extends ChromeBaseSettingsFragment
        implements PersonalDataManager.PersonalDataManagerObserver {
    @Override
    protected void notifyPreferencesUpdated() {
        Preference loyaltyCardsPref =
                getPreferenceScreen()
                        .findPreference(AutofillPaymentMethodsFragment.PREF_LOYALTY_CARDS);
        if (loyaltyCardsPref != null) {
            getPreferenceScreen().removePreference(loyaltyCardsPref);
        }
        super.notifyPreferencesUpdated();
    }

    public static final ChromeBaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new ChromeBaseSearchIndexProvider(AutofillPaymentMethodsFragment.class.getName(), 0) {
                @Override
                public void updateDynamicPreferences(
                        Context context, SettingsIndexData indexData, Profile profile) {
                    AutofillPaymentMethodsFragment.SEARCH_INDEX_DATA_PROVIDER
                            .updateDynamicPreferences(context, indexData, profile);
                    indexData.removeEntryForKey(
                            AutofillPaymentMethodsFragment.class.getName(),
                            AutofillPaymentMethodsFragment.PREF_LOYALTY_CARDS);
                }
            };
}
