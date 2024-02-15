/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.res.Resources;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Fragment adapter for the main sections of Brave Wallet displayed by {@link BraveWalletActivity}.
 */
public class CryptoFragmentPageAdapter extends FragmentStateAdapter {
    public static final int PORTFOLIO_FRAGMENT_POSITION = 0;
    public static final int TRANSACTIONS_ACTIVITY_FRAGMENT_POSITION = 1;
    public static final int ACCOUNTS_FRAGMENT_POSITION = 2;
    public static final int MARKET_FRAGMENT_POSITION = 3;

    private final List<String> mTitles;

    public CryptoFragmentPageAdapter(@NonNull final FragmentActivity activity) {
        super(activity);
        Resources resources = activity.getResources();
        mTitles = new ArrayList<String>(Arrays.asList(resources.getString(R.string.brave_wallet_activity),
                resources.getString(R.string.accounts), resources.getString(R.string.explore)));
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        switch (position) {
            default:
                throw new IllegalStateException(
                        String.format("No fragment found for position %d.", position));
        }
    }

    @Override
    public int getItemCount() {
        return mTitles.size();
    }
}
