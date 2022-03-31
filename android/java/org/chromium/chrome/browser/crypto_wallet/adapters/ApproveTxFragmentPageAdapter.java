/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.app.Activity;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.TxDetailsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.TxFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class ApproveTxFragmentPageAdapter extends FragmentStatePagerAdapter {
    private List<String> mTitles;
    private TransactionInfo mTxInfo;
    private String mAsset;
    private int mDecimals;
    private String mChainSymbol;
    private int mChainDecimals;
    private double mTotalPrice;

    public ApproveTxFragmentPageAdapter(FragmentManager fm, TransactionInfo txInfo, String asset,
            int decimals, String chainSymbol, int chainDecimals, double totalPrice,
            Activity activity) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        mTxInfo = txInfo;
        mAsset = asset;
        mDecimals = decimals;
        mChainSymbol = chainSymbol;
        mChainDecimals = chainDecimals;
        mTotalPrice = totalPrice;
        mTitles = new ArrayList<>(Arrays.asList(activity.getText(R.string.transaction).toString(),
                activity.getText(R.string.transaction_details).toString()));
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        if (position == 0) {
            return TxFragment.newInstance(
                    mTxInfo, mAsset, mDecimals, mChainSymbol, mChainDecimals, mTotalPrice);
        } else {
            return TxDetailsFragment.newInstance(mTxInfo);
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
}
