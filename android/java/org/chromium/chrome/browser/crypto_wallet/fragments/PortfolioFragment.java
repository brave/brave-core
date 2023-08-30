/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.PortfolioFragmentStateAdapter;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;

import java.util.List;

/**
 * Main section of Brave Wallet showing Asset list and NFT grid view.
 */
public class PortfolioFragment extends Fragment implements ApprovedTxObserver {
    private static final String TAG = "PortfolioFragment";

    private TabLayout mTabLayout;
    private ViewPager2 mViewPager;

    private PortfolioFragmentStateAdapter mViewPagerAdapter;

    private WalletModel mWalletModel;
    private List<TransactionInfo> mPendingTxs;
    private TransactionInfo mCurrentPendingTx;

    public static PortfolioFragment newInstance() {
        return new PortfolioFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate", e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        if (mWalletModel != null) {
            // Show pending transactions fab to process pending txs
            mWalletModel.getCryptoModel().getPendingTransactions().observe(
                    getViewLifecycleOwner(), transactions -> {
                        mPendingTxs = transactions;
                        if (mCurrentPendingTx == null && mPendingTxs.size() > 0) {
                            mCurrentPendingTx = mPendingTxs.get(0);
                        } else if (mPendingTxs.size() == 0) {
                            mCurrentPendingTx = null;
                        }
                        updatePendingTxNotification();
                    });
        }
        return inflater.inflate(org.chromium.chrome.R.layout.fragment_portfolio, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mTabLayout = view.findViewById(R.id.portfolio_tab_layout);
        mViewPager = view.findViewById(R.id.portfolio_view_pager);

        mViewPagerAdapter = new PortfolioFragmentStateAdapter(this);
        mViewPager.setAdapter(mViewPagerAdapter);
        mViewPager.setUserInputEnabled(false);
        new TabLayoutMediator(mTabLayout, mViewPager,
                (tab, position) -> tab.setText(mViewPagerAdapter.getPageTitle(position)))
                .attach();
    }

    public void callAnotherApproveDialog() {
        if (!hasPendingTx() || mWalletModel == null) {
            return;
        }
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(mCurrentPendingTx);
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
        approveTxBottomSheetDialogFragment.show(
                getParentFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private boolean hasPendingTx() {
        return mCurrentPendingTx != null;
    }

    private void updatePendingTxNotification() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            ((BraveWalletActivity) activity).showPendingTxNotification(hasPendingTx());
        }
    }

    private void updateNextPendingTx() {
        if (mCurrentPendingTx != null) {
            for (TransactionInfo info : mPendingTxs) {
                if (!mCurrentPendingTx.id.equals(info.id)
                        && info.txStatus == TransactionStatus.UNAPPROVED) {
                    mCurrentPendingTx = info;
                    return;
                }
            }
            mCurrentPendingTx = null;
        } else if (mPendingTxs.size() > 0) {
            mCurrentPendingTx = mPendingTxs.get(0);
        }
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String txId) {
        updatePendingTxNotification();
        updateNextPendingTx();
        callAnotherApproveDialog();
    }

    @Override
    public void onTxPending(String txId) {
        /* No op. */
    }
}
