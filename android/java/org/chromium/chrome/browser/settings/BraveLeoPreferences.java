/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.ai_chat.mojom.PremiumStatus;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoCMHelper;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveLeoPreferences extends BravePreferenceFragment {
    private static final String TAG = "BraveLeoPreferences";
    private static final String PREF_LINK_SUBSCRIPTION = "link_subscription";
    private static final String LINK_SUBSCRIPTION_URL = "https://account.brave.com";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        requireActivity().setTitle(R.string.menu_brave_leo);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_leo_preferences);

        BraveLeoCMHelper.getInstance(getProfile())
                .getPremiumStatus(
                        status -> {
                            if (status != PremiumStatus.ACTIVE) {
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
                                            } catch (
                                                    BraveActivity.BraveActivityNotFoundException
                                                            e) {
                                                Log.e(
                                                        TAG,
                                                        "Error while opening subscription tab.",
                                                        e);
                                            }
                                            return true;
                                        });
                            }
                        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        BraveLeoCMHelper.getInstance(getProfile()).destroy();
    }
}
