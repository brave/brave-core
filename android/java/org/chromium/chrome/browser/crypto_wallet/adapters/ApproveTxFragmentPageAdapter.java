/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.SolanaTxDetailsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.TxDetailsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.TxFragment;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class ApproveTxFragmentPageAdapter extends FragmentStatePagerAdapter {
    private List<String> mTitles;
    private TransactionInfo mTxInfo;
    private NetworkInfo mSelectedNetwork;
    private AccountInfo[] mAccounts;
    private HashMap<String, Double> mAssetPrices;
    private BlockchainToken[] mFullTokenList;
    private HashMap<String, Double> mNativeAssetsBalances;
    private HashMap<String, HashMap<String, Double>> mBlockchainTokensBalances;
    private boolean mUpdateTxObjectManually;
    private long mSolanaEstimatedTxFee;
    private Context mContext;
    private Fragment mDetailsFragment;

    public ApproveTxFragmentPageAdapter(
            FragmentManager fm,
            TransactionInfo txInfo,
            NetworkInfo selectedNetwork,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            Activity activity,
            boolean updateTxObjectManually,
            long solanaEstimatedTxFee) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        mTxInfo = txInfo;
        mSelectedNetwork = selectedNetwork;
        mAccounts = accounts;
        mAssetPrices = assetPrices;
        mFullTokenList = fullTokenList;
        mNativeAssetsBalances = nativeAssetsBalances;
        mBlockchainTokensBalances = blockchainTokensBalances;
        mContext = activity;
        mTitles =
                new ArrayList<>(
                        Arrays.asList(
                                activity.getText(R.string.transaction).toString(),
                                activity.getText(R.string.transaction_details).toString()));
        mUpdateTxObjectManually = updateTxObjectManually;
        mSolanaEstimatedTxFee = solanaEstimatedTxFee;
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        if (position == 0) {
            return TxFragment.newInstance(
                    mTxInfo,
                    mSelectedNetwork,
                    mAccounts,
                    mAssetPrices,
                    mFullTokenList,
                    mNativeAssetsBalances,
                    mBlockchainTokensBalances,
                    mUpdateTxObjectManually,
                    mSolanaEstimatedTxFee);
        } else {
            if (mDetailsFragment == null) {
                if (TransactionUtils.isSolanaTx(mTxInfo)) {
                    mDetailsFragment = SolanaTxDetailsFragment.newInstance(mTxInfo, mContext);
                } else {
                    mDetailsFragment = TxDetailsFragment.newInstance(mTxInfo);
                }
            }
            return mDetailsFragment;
        }
    }

    @Override
    public void notifyDataSetChanged() {
        mDetailsFragment = null;
        super.notifyDataSetChanged();
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
