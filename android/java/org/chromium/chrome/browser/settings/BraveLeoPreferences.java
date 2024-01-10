/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.preference.Preference;

import org.chromium.ai_chat.mojom.PremiumStatus;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoCMHelper;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.brave_leo.BraveLeoUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveLeoPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "BraveLeoPreferences";
    private static final String PREF_LINK_SUBSCRIPTION = "link_subscription";
    private static final String PREF_AUTOCOMPLETE = "autocomplete_switch";
    private static final String LINK_SUBSCRIPTION_URL =
            "https://account.brave.com?intent=connect-receipt&product=leo";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        requireActivity().setTitle(R.string.menu_brave_leo);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_leo_preferences);

        BraveLeoUtils.verifySubscription(
                (subscriptionActive) -> {
                    checkLinkPurchase();
                });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        BraveLeoCMHelper.getInstance(getProfile()).destroy();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Preference autocomplete = findPreference(PREF_AUTOCOMPLETE);
        if (autocomplete != null) {
            autocomplete.setOnPreferenceChangeListener(this);
            if (autocomplete instanceof ChromeSwitchPreference) {
                ((ChromeSwitchPreference) autocomplete)
                        .setChecked(
                                ChromeSharedPreferences.getInstance()
                                        .readBoolean(
                                                BravePreferenceKeys.BRAVE_LEO_AUTOCOMPLETE, true));
            }
        }
    }

    private void checkLinkPurchase() {
        BraveLeoCMHelper.getInstance(getProfile())
                .getPremiumStatus(
                        (status, info) -> {
                            if (status == PremiumStatus.ACTIVE
                                    || !BraveLeoPrefUtils.getIsSubscriptionActive(getProfile())) {
                                return;
                            }
                            ChromeBasePreference link_subscription =
                                    findPreference(PREF_LINK_SUBSCRIPTION);
                            if (link_subscription == null) {
                                Log.e(TAG, "Subscription pref is null");
                                return;
                            }
                            link_subscription.setVisible(true);
                            link_subscription.setOnPreferenceClickListener(
                                    preference -> {
                                        try {
                                            BraveActivity.getBraveActivity()
                                                    .openNewOrSelectExistingTab(
                                                            LINK_SUBSCRIPTION_URL, true);
                                            TabUtils.bringChromeTabbedActivityToTheTop(
                                                    getActivity());
                                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                                            Log.e(TAG, "Error while opening subscription tab.", e);
                                        }
                                        return true;
                                    });
                        });
    }

    @Override
    public boolean onPreferenceChange(@NonNull Preference preference, Object o) {
        String key = preference.getKey();
        if (PREF_AUTOCOMPLETE.equals(key)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_LEO_AUTOCOMPLETE, (boolean) o);
        }

        return true;
    }
}
