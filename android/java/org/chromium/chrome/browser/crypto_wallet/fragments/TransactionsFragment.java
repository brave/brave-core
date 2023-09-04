/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.TransactionsModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.app.shimmer.ShimmerFrameLayout;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.base.ViewUtils;

import java.lang.ref.WeakReference;
import java.util.Collections;
import java.util.List;

public class TransactionsFragment extends Fragment implements OnWalletListItemClick {
    private static final String TAG = "TransactionsFragment";
    private WalletModel mWalletModel;
    private TransactionsModel mTransactionsModel;
    private RecyclerView mRvTransactions;
    private WalletCoinAdapter mWalletTxAdapter;
    private List<WalletListItemModel> mWalletListItemModelList;
    private TextView mTvEmptyListLabel;
    private TextView mTvEmptyListDesc;
    private ShimmerFrameLayout mShimmerLoading;
    private ViewGroup mShimmerItems;

    public static TransactionsFragment newInstance() {
        return new TransactionsFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mTransactionsModel = mWalletModel.getCryptoModel().createTransactionModel();
            mTransactionsModel.update(
                    new WeakReference<>((BraveWalletBaseActivity) requireActivity()));
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }
        mWalletListItemModelList = Collections.emptyList();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_transactions, container, false);
        mRvTransactions = view.findViewById(R.id.frag_tx_rv_transactions);
        mShimmerLoading = view.findViewById(R.id.frag_tx_shimmer);
        mTvEmptyListLabel = view.findViewById(R.id.frag_tx_empty_label);
        mTvEmptyListDesc = view.findViewById(R.id.frag_tx_empty_desc);
        mWalletTxAdapter = new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        mWalletTxAdapter.setOnWalletListItemClick(TransactionsFragment.this);
        mWalletTxAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        mRvTransactions.setAdapter(mWalletTxAdapter);
        mShimmerItems = view.findViewById(R.id.ll_shimmer_items);
        int shimmerSkeletonRows =
                AndroidUtils.getSkeletonRowCount(ViewUtils.dpToPx(requireContext(), 50));
        for (int i = 0; i < shimmerSkeletonRows; i++) {
            inflater.inflate(R.layout.shimmer_skeleton_item, mShimmerItems, true);
        }
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setUpObservers();
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        if (txInfo.txStatus == TransactionStatus.UNAPPROVED) {
            ApproveTxBottomSheetDialogFragment approveTx =
                    ApproveTxBottomSheetDialogFragment.newInstance(txInfo);
            approveTx.show(getChildFragmentManager(), TAG);
        } else {
            WalletListItemModel txModel = JavaUtils.find(
                    mWalletListItemModelList, tx -> tx.getTransactionInfo().id.equals(txInfo.id));
            TransactionDetailsSheetFragment.newInstance(txModel).show(
                    getChildFragmentManager(), TAG);
        }
    }

    private void setUpObservers() {
        if (JavaUtils.anyNull(mTransactionsModel)) return;
        mTransactionsModel.mParsedTransactions.observe(
                getViewLifecycleOwner(), walletListItemModelList -> {
                    if (walletListItemModelList == null) return;

                    mWalletListItemModelList = walletListItemModelList;
                    updateTransactionList(walletListItemModelList);
                    if (walletListItemModelList.isEmpty()) {
                        AndroidUtils.gone(mRvTransactions);
                        AndroidUtils.show(mTvEmptyListLabel, mTvEmptyListDesc);
                    } else {
                        AndroidUtils.show(mRvTransactions);
                        AndroidUtils.gone(mTvEmptyListLabel, mTvEmptyListDesc);
                    }
                });
        mTransactionsModel.mIsLoading.observe(getViewLifecycleOwner(), isLoading -> {
            if (isLoading) {
                mShimmerLoading.showShimmer(true);
                AndroidUtils.show(mShimmerItems);
            } else {
                AndroidUtils.gone(mShimmerItems);
                mShimmerLoading.hideShimmer();
            }
        });
    }

    @SuppressLint("NotifyDataSetChanged")
    private void updateTransactionList(List<WalletListItemModel> walletListItemModelList) {
        mWalletTxAdapter.setWalletListItemModelList(walletListItemModelList);
        mWalletTxAdapter.notifyDataSetChanged();
    }
}
