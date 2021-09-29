/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.ApproveTxFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.Locale;

public class ApproveTxBottomSheetDialogFragment extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = ApproveTxBottomSheetDialogFragment.class.getName();

    private String mNetworkName;
    private TransactionInfo mTxInfo;
    private String mAccountName;
    private int mAccountPic;
    private String mTo;
    private String mTxType;
    private String mAsset;
    private boolean mRejected;
    private boolean mApproved;
    private double mTotalPrice;

    public static ApproveTxBottomSheetDialogFragment newInstance(String networkName,
            TransactionInfo txInfo, String accountName, int accountPic, String txType,
            String asset) {
        return new ApproveTxBottomSheetDialogFragment(
                networkName, txInfo, accountName, accountPic, txType, asset);
    }

    ApproveTxBottomSheetDialogFragment(String networkName, TransactionInfo txInfo,
            String accountName, int accountPic, String txType, String asset) {
        mNetworkName = networkName;
        mTxInfo = txInfo;
        mAccountName = accountName;
        mAccountPic = accountPic;
        mTxType = txType;
        mAsset = asset;
        mRejected = false;
        mApproved = false;
    }

    private AssetRatioController getAssetRatioController() {
        Activity activity = getActivity();
        if (activity instanceof BuySendSwapActivity) {
            return ((BuySendSwapActivity) activity).getAssetRatioController();
        }

        return null;
    }

    private EthTxController getEthTxController() {
        Activity activity = getActivity();
        if (activity instanceof BuySendSwapActivity) {
            return ((BuySendSwapActivity) activity).getEthTxController();
        }

        return null;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            ApproveTxBottomSheetDialogFragment fragment =
                    (ApproveTxBottomSheetDialogFragment) manager.findFragmentByTag(
                            ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("ApproveTxBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mRejected || mApproved) {
            return;
        }
        rejectTransaction(false);
    }

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.approve_tx_bottom_sheet, null);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
        TextView networkName = view.findViewById(R.id.network_name);
        networkName.setText(mNetworkName);
        ImageView icon = (ImageView) view.findViewById(R.id.account_picture);
        icon.setImageResource(mAccountPic);
        String valueToConvert = mTxInfo.txData.baseData.value;
        String to = mTxInfo.txData.baseData.to;
        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER && mTxInfo.txArgs.length > 1) {
            valueToConvert = mTxInfo.txArgs[1];
            to = mTxInfo.txArgs[0];
        }
        TextView fromTo = view.findViewById(R.id.from_to);
        fromTo.setText(String.format(getResources().getString(R.string.crypto_wallet_from_to),
                mAccountName, Utils.stripAccountAddress(to)));
        TextView txType = view.findViewById(R.id.tx_type);
        txType.setText(mTxType);
        TextView amountAsset = view.findViewById(R.id.amount_asset);
        amountAsset.setText(String.format(
                getResources().getString(R.string.crypto_wallet_amount_asset),
                String.format(Locale.getDefault(), "%.4f", Utils.fromHexWei(valueToConvert)),
                mAsset));
        AssetRatioController assetRatioController = getAssetRatioController();
        if (assetRatioController != null) {
            String[] assets = {mAsset.toLowerCase(Locale.getDefault())};
            String[] toCurr = {"usd"};
            assetRatioController.getPrice(
                    assets, toCurr, AssetPriceTimeframe.LIVE, (success, values) -> {
                        String valueFiat = "0";
                        if (values.length != 0) {
                            valueFiat = values[0].price;
                        }
                        String valueAsset = mTxInfo.txData.baseData.value;
                        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER
                                && mTxInfo.txArgs.length > 1) {
                            valueAsset = mTxInfo.txArgs[1];
                        }
                        double value = Utils.fromHexWei(valueAsset);
                        double price = Double.valueOf(valueFiat);
                        mTotalPrice = value * price;
                        TextView amountFiat = view.findViewById(R.id.amount_fiat);
                        amountFiat.setText(String.format(
                                getResources().getString(R.string.crypto_wallet_amount_fiat),
                                String.format(Locale.getDefault(), "%.2f", mTotalPrice)));
                        ViewPager viewPager = view.findViewById(R.id.navigation_view_pager);
                        ApproveTxFragmentPageAdapter adapter =
                                new ApproveTxFragmentPageAdapter(getChildFragmentManager(), mTxInfo,
                                        mAsset, mTotalPrice, getActivity());
                        viewPager.setAdapter(adapter);
                        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
                        TabLayout tabLayout = view.findViewById(R.id.tabs);
                        tabLayout.setupWithViewPager(viewPager);
                    });
        }

        Button reject = view.findViewById(R.id.reject);
        reject.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                rejectTransaction(true);
            }
        });

        Button approve = view.findViewById(R.id.approve);
        approve.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                approveTransaction();
            }
        });
    }

    private void rejectTransaction(boolean dismiss) {
        EthTxController ethTxController = getEthTxController();
        if (ethTxController == null) {
            return;
        }
        ethTxController.rejectTransaction(mTxInfo.id, success -> {
            assert success;
            if (!success || !dismiss) {
                return;
            }
            mRejected = true;
            dismiss();
        });
    }

    private void approveTransaction() {
        EthTxController ethTxController = getEthTxController();
        if (ethTxController == null) {
            return;
        }
        ethTxController.approveTransaction(mTxInfo.id, success -> {
            assert success;
            if (!success) {
                return;
            }
            mApproved = true;
            dismiss();
        });
    }
}
