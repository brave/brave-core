/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletResetActivity;
import org.chromium.chrome.R;

/**
 * The preference used to reset Brave Wallet.
 */
public class BraveWalletResetPreference
        extends Preference implements Preference.OnPreferenceClickListener {

    private int mPrefAccentColor;

    /**
     * Constructor for BraveWalletResetPreference.
     */
    public BraveWalletResetPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        Resources resources = getContext().getResources();
        mPrefAccentColor =
                ApiCompatibilityUtils.getColor(resources, R.color.wallet_error_text_color);
        setOnPreferenceClickListener(this);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        TextView titleView = (TextView) holder.findViewById(android.R.id.title);
        titleView.setTextColor(mPrefAccentColor);
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        showBraveWalletResetDialog();
        return true;
    }

    private void showBraveWalletResetDialog() {
        Intent braveWalletResetIntent = new Intent(getContext(), BraveWalletResetActivity.class);
        getContext().startActivity(braveWalletResetIntent);
    }
}
