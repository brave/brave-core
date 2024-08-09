/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import android.os.Bundle;
import android.text.SpannableString;
import android.view.View;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ClickableSpansTextMessagePreference;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

public class BraveClearBrowsingDataFragmentAdvanced extends ClearBrowsingDataFragmentAdvanced {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        getPreferenceScreen()
                .addPreference(
                        BraveRewardsHelper.isRewardsEnabled()
                                ? buildResetBraveRewardsDataPref()
                                : buildClearBraveAdsDataPref());
    }

    private ClickableSpansTextMessagePreference buildResetBraveRewardsDataPref() {
        SpannableString resetBraveRewardsDataText =
                SpanApplier.applySpans(
                        getContext().getString(R.string.reset_brave_rewards_data),
                        new SpanInfo(
                                "<link1>",
                                "</link1>",
                                new NoUnderlineClickableSpan(
                                        requireContext(), resetBraveRewardsDataCallback())));

        ClickableSpansTextMessagePreference resetBraveRewardsDataPref =
                new ClickableSpansTextMessagePreference(getContext(), null);
        resetBraveRewardsDataPref.setSummary(resetBraveRewardsDataText);
        return resetBraveRewardsDataPref;
    }

    private ClickableSpansTextMessagePreference buildClearBraveAdsDataPref() {
        SpannableString clearBraveAdsDataText =
                SpanApplier.applySpans(
                        getContext().getString(R.string.clear_brave_ads_data),
                        new SpanInfo(
                                "<link1>",
                                "</link1>",
                                new NoUnderlineClickableSpan(
                                        requireContext(), clearBraveAdsDataCallback())));

        ClickableSpansTextMessagePreference clearBraveAdsDataPref =
                new ClickableSpansTextMessagePreference(getContext(), null);
        clearBraveAdsDataPref.setSummary(clearBraveAdsDataText);
        return clearBraveAdsDataPref;
    }

    private Callback<View> resetBraveRewardsDataCallback() {
        return (view) -> {
            try {
                TabUtils.openUrlInNewTab(false, BraveActivity.BRAVE_REWARDS_RESET_PAGE);
                TabUtils.bringChromeTabbedActivityToTheTop(BraveActivity.getBraveActivity());
            } catch (BraveActivity.BraveActivityNotFoundException e) {
            }
        };
    }

    private Callback<View> clearBraveAdsDataCallback() {
        return (view) -> {
            Profile profile = getProfile();
            if (profile != null) {
                BraveAdsNativeHelper.nativeClearData(profile);
            }

            if (getActivity() != null) {
                getActivity().finish();
            }
        };
    }
}
