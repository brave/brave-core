/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.support.v7.preference.PreferenceDialogFragmentCompat;
import android.view.View;

import org.chromium.chrome.browser.BraveRewardsNativeWorker;

/**
 * The dialog used to reset Brave Rewards.
 */
public class BraveRewardsResetPreferenceDialog extends PreferenceDialogFragmentCompat {
    public static final String TAG = "BraveRewardsResetPreferenceDialog";

    public static BraveRewardsResetPreferenceDialog newInstance(
            BraveRewardsResetPreference preference) {
        BraveRewardsResetPreferenceDialog fragment = new BraveRewardsResetPreferenceDialog();
        Bundle bundle = new Bundle(1);
        bundle.putString(PreferenceDialogFragmentCompat.ARG_KEY, preference.getKey());
        fragment.setArguments(bundle);
        return fragment;
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
    }

    @Override
    public void onDialogClosed(boolean positive) {
        if (positive) {
            BraveRewardsNativeWorker.getInstance().SetRewardsMainEnabled(false);
        }
    }
}
