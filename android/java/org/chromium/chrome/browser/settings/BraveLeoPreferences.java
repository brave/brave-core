/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import org.chromium.ai_chat.mojom.PremiumStatus;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.brave_leo.BraveLeoMojomHelper;
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
    private static final String PREF_MANAGE_SUBSCRIPTION = "subscription_manage";
    private static final String PREF_AUTOCOMPLETE = "autocomplete_switch";
    private static final String PREF_SUBSCRIPTION_CATEGORY = "subscription_category";
    private static final String PREF_DEFAULT_MODEL = "default_model";
    private static final String LINK_SUBSCRIPTION_URL =
            "https://account.brave.com?intent=connect-receipt&product=leo";

    private PreferenceCategory mSubscriptionCategory;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        requireActivity().setTitle(R.string.menu_brave_leo);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_leo_preferences);
        mSubscriptionCategory = findPreference(PREF_SUBSCRIPTION_CATEGORY);
        assert mSubscriptionCategory != null;

        BraveLeoUtils.verifySubscription(
                (subscriptionActive) -> {
                    checkLinkPurchase();
                });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        BraveLeoMojomHelper.getInstance(getProfile()).destroy();
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

    private void setModel() {
        BraveLeoMojomHelper.getInstance(getProfile())
                .getModels(
                        (models -> {
                            Preference pref = findPreference(PREF_DEFAULT_MODEL);
                            if (pref == null) {
                                Log.e(TAG, "Default model pref is null");
                                return;
                            }
                            pref.setSummary(BraveLeoUtils.getDefaultModelName(models));
                        }));
    }

    private void checkLinkPurchase() {
        BraveLeoMojomHelper.getInstance(getProfile())
                .getPremiumStatus(
                        (status, info) -> {
                            if (status == PremiumStatus.ACTIVE
                                    || !BraveLeoPrefUtils.getIsSubscriptionActive(getProfile())) {
                                return;
                            }
                            ChromeBasePreference linkSubscription =
                                    findPreference(PREF_LINK_SUBSCRIPTION);
                            ChromeBasePreference manageSubscription =
                                    findPreference(PREF_MANAGE_SUBSCRIPTION);
                            if (linkSubscription == null
                                    || manageSubscription == null
                                    || mSubscriptionCategory == null) {
                                Log.e(TAG, "Subscription pref is null");
                                return;
                            }
                            mSubscriptionCategory.setVisible(true);
                            linkSubscription.setOnPreferenceClickListener(
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
                            manageSubscription.setOnPreferenceClickListener(
                                    preference -> {
                                        Intent browserIntent =
                                                new Intent(
                                                        Intent.ACTION_VIEW,
                                                        Uri.parse(
                                                                InAppPurchaseWrapper
                                                                        .MANAGE_SUBSCRIPTION_PAGE));
                                        getActivity().startActivity(browserIntent);
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

    @Override
    public void onResume() {
        super.onResume();
        setModel();
    }
}
