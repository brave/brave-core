/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.model.AccountSelectorItemModel;
import org.chromium.chrome.browser.crypto_wallet.presenters.Amount;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TransactionDetailsSheetFragment extends WalletBottomSheetDialogFragment {
    public static final String TAG = TransactionDetailsSheetFragment.class.getName();
    private ExecutorService mExecutor;
    private Handler mHandler;
    private WalletModel mWalletModel;
    private AccountSelectorItemModel mAccountSelectorItemModel;
    private TransactionInfo mTxInfo;
    private ParsedTransaction mParsedTx;

    public static TransactionDetailsSheetFragment newInstance(
            AccountSelectorItemModel accountSelectorItemModel) {
        return new TransactionDetailsSheetFragment(accountSelectorItemModel);
    }

    private TransactionDetailsSheetFragment(AccountSelectorItemModel accountSelectorItemModel) {
        mAccountSelectorItemModel = accountSelectorItemModel;
        mTxInfo = mAccountSelectorItemModel.getTransactionInfo();
        mParsedTx = mAccountSelectorItemModel.getParsedTx();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            TransactionDetailsSheetFragment fragment =
                    (TransactionDetailsSheetFragment)
                            manager.findFragmentByTag(TransactionDetailsSheetFragment.TAG);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e(TAG, "show", e);
        }
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            registerKeyringObserver(mWalletModel.getKeyringModel());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return LayoutInflater.from(getContext())
                .inflate(R.layout.tx_details_bottom_sheet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (JavaUtils.anyNull(mWalletModel)) return;
        ImageView icon = view.findViewById(R.id.account_picture);
        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, icon, mAccountSelectorItemModel.getAccountInfo(), true);
        updateTxHeaderDetails(view);
        updateTxDetails(view);
    }

    private void updateTxHeaderDetails(View view) {
        if (JavaUtils.anyNull(mWalletModel)) return;
        TextView fromTo = view.findViewById(R.id.from_to);
        ImageView icon = view.findViewById(R.id.account_picture);
        TextView txType = view.findViewById(R.id.tx_type);
        TextView amountFiat = view.findViewById(R.id.amount_fiat);
        TextView amountAsset = view.findViewById(R.id.amount_asset);

        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, icon, mAccountSelectorItemModel.getAccountInfo(), true);

        if (mTxInfo.originInfo != null && URLUtil.isValidUrl(mTxInfo.originInfo.originSpec)) {
            TextView domain = view.findViewById(R.id.domain);
            domain.setVisibility(View.VISIBLE);
            domain.setText(Utils.geteTldSpanned(mTxInfo.originInfo));
        }

        if (mParsedTx.getType() == TransactionType.ERC20_APPROVE) {
            txType.setText(
                    String.format(
                            getResources().getString(R.string.activate_erc20),
                            mParsedTx.getSymbol()));
        } else if (mParsedTx.getIsSwap()) {
            txType.setText(getResources().getString(R.string.swap));
        } else if (mParsedTx.isSolanaDappTransaction) {
            txType.setText(R.string.brave_wallet_approve_transaction);
        } else {
            txType.setText(getResources().getString(R.string.send));
        }

        if (mParsedTx.isSolanaDappTransaction) {
            AndroidUtils.gone(amountFiat, amountAsset);
        } else {
            amountFiat.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_amount_fiat),
                            String.format(Locale.ENGLISH, "%.2f", mParsedTx.getFiatTotal())));
            String amountText =
                    String.format(
                            getResources().getString(R.string.crypto_wallet_amount_asset),
                            mParsedTx.formatValueToDisplay(),
                            mParsedTx.getSymbol());

            if (mTxInfo.txType == TransactionType.ERC721_TRANSFER_FROM
                    || mTxInfo.txType == TransactionType.ERC721_SAFE_TRANSFER_FROM) {
                amountText = Utils.tokenToString(mParsedTx.getErc721BlockchainToken());
                amountFiat.setVisibility(View.GONE); // Display NFT values in the future
            }
            amountAsset.setText(amountText);
        }

        String accountName = mAccountSelectorItemModel.getAccountInfo().name;
        if (mParsedTx.getSender() != null
                && !mParsedTx.getSender().equals(mParsedTx.getRecipient())) {
            String recipient =
                    TextUtils.isEmpty(mParsedTx.getRecipient()) ? "..." : mParsedTx.getRecipient();
            fromTo.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_from_to),
                            accountName,
                            mParsedTx.getSender(),
                            "->",
                            recipient));
        } else {
            fromTo.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_from_to),
                            accountName,
                            mParsedTx.getSender(),
                            "",
                            ""));
        }
    }

    private void updateTxDetails(View view) {
        RecyclerView txDetails = view.findViewById(R.id.rv_tx_details);
        List<TwoLineItemRecyclerViewAdapter.TwoLineItem> items = new ArrayList<>();

        double totalGas = mParsedTx.getGasFee();
        if (totalGas > 0) {
            String gasFiatAndCrypto =
                    String.format(
                                    getResources().getString(R.string.crypto_wallet_gas_fee_amount),
                                    String.format(Locale.ENGLISH, "%.8f", totalGas),
                                    mParsedTx.getSymbol())
                            + System.getProperty(WalletConstants.LINE_SEPARATOR)
                            + String.format(
                                    getResources().getString(R.string.crypto_wallet_amount_fiat),
                                    new Amount(mParsedTx.getFiatTotal()).toStringFormat());
            items.add(
                    new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                            getString(R.string.brave_wallet_allow_spend_transaction_fee),
                            gasFiatAndCrypto));
            items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        }
        if (mParsedTx.marketPrice > 0) {
            items.add(
                    new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                            getString(R.string.market_price_text),
                            String.format(Locale.ENGLISH, "$%,.6f", mParsedTx.marketPrice)));
            items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        }
        items.add(
                new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                        getString(R.string.date_text),
                        WalletUtils.timeDeltaToDateString(mParsedTx.getCreatedTime())));
        items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        items.add(
                new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                        getString(R.string.network_text),
                        mAccountSelectorItemModel.getAssetNetwork().chainName));
        items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        items.add(
                new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                        getString(R.string.status),
                        mAccountSelectorItemModel.getTxStatus(),
                        (title, subtitle) -> {
                            subtitle.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                    new BitmapDrawable(
                                            getResources(),
                                            mAccountSelectorItemModel.getTxStatusBitmap()),
                                    null,
                                    null,
                                    null);
                        }));

        txDetails.setAdapter(
                new TwoLineItemRecyclerViewAdapter(
                        items, TwoLineItemRecyclerViewAdapter.ADAPTER_VIEW_ORIENTATION.HORIZONTAL));
    }
}
