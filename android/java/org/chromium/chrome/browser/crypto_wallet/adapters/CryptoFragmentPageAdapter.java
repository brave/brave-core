/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.content.res.Resources;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.AccountsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.MarketFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.NftGridFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.PortfolioFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.TransactionsFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CryptoFragmentPageAdapter extends FragmentStatePagerAdapter {
    private static final int PORTFOLIO_FRAGMENT_POSITION = 0;
    private static final int NFT_GRID_FRAGMENT_POSITION = 1;
    private static final int TRANSACTIONS_ACTIVITY_FRAGMENT_POSITION = 2;
    private static final int ACCOUNTS_FRAGMENT_POSITION = 3;
    private static final int MARKET_FRAGMENT_POSITION = 4;

    private final List<String> mTitles;

    private PortfolioFragment mCurrentPortfolioFragment;

    public CryptoFragmentPageAdapter(FragmentManager fm, Context context) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        Resources resources = context.getResources();
        mTitles = new ArrayList<>(Arrays.asList(resources.getString(R.string.portfolio),
                resources.getString(R.string.brave_wallet_nfts),
                resources.getString(R.string.brave_wallet_activity),
                resources.getString(R.string.accounts), resources.getString(R.string.market)));
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        switch (position) {
            case PORTFOLIO_FRAGMENT_POSITION:
                mCurrentPortfolioFragment = PortfolioFragment.newInstance();
                return mCurrentPortfolioFragment;
            case NFT_GRID_FRAGMENT_POSITION:
                return NftGridFragment.newInstance();
            case TRANSACTIONS_ACTIVITY_FRAGMENT_POSITION:
                return TransactionsFragment.newInstance();
            case ACCOUNTS_FRAGMENT_POSITION:
                return AccountsFragment.newInstance();
            case MARKET_FRAGMENT_POSITION:
                return MarketFragment.newInstance();
            default:
                throw new IllegalStateException(
                        String.format("No fragment found for position %d.", position));
        }
    }

    @Override
    public int getCount() {
        return mTitles.size();
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return mTitles.get(position);
    }

    public PortfolioFragment getCurrentPortfolioFragment() {
        return mCurrentPortfolioFragment;
    }
}
