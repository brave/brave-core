/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.Locale;

public class TxFragment extends Fragment {
    private TransactionInfo mTxInfo;
    private String mAsset;
    private double mTotalPrice;

    public static TxFragment newInstance(TransactionInfo txInfo, String asset, double totalPrice) {
        return new TxFragment(txInfo, asset, totalPrice);
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

    private TxFragment(TransactionInfo txInfo, String asset, double totalPrice) {
        mTxInfo = txInfo;
        mAsset = asset;
        mTotalPrice = totalPrice;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_transaction, container, false);

        setupView(view);

        TextView editGasFee = view.findViewById(R.id.edit_gas_fee);
        editGasFee.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Dialog dialog = new Dialog(getActivity());
                dialog.setContentView(R.layout.brave_wallet_edit_gas);
                dialog.show();

                EditText gasFeeEdit = dialog.findViewById(R.id.gas_fee_edit);
                gasFeeEdit.setText(String.format(Locale.getDefault(), "%.0f",
                        Utils.fromHexWeiToGWEI(mTxInfo.txData.baseData.gasPrice)));

                EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                gasLimitEdit.setText(String.format(Locale.getDefault(), "%.0f",
                        Utils.fromHexWeiToGWEI(mTxInfo.txData.baseData.gasLimit)));

                Button cancel = dialog.findViewById(R.id.cancel);
                cancel.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dialog.dismiss();
                    }
                });
                Button ok = dialog.findViewById(R.id.ok);
                ok.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        EthTxController ethTxController = getEthTxController();
                        assert ethTxController != null;
                        if (ethTxController == null) {
                            dialog.dismiss();

                            return;
                        }
                        mTxInfo.txData.baseData.gasPrice =
                                Utils.toHexWeiFromGWEI(gasFeeEdit.getText().toString());
                        mTxInfo.txData.baseData.gasLimit =
                                Utils.toHexWeiFromGWEI(gasLimitEdit.getText().toString());
                        ethTxController.setGasPriceAndLimitForUnapprovedTransaction(mTxInfo.id,
                                mTxInfo.txData.baseData.gasPrice, mTxInfo.txData.baseData.gasLimit,
                                success -> {
                                    if (!success) {
                                        return;
                                    }
                                    setupView(view);
                                    dialog.dismiss();
                                });
                    }
                });
            }
        });

        return view;
    }

    private void setupView(View view) {
        TextView gasFeeAmount = view.findViewById(R.id.gas_fee_amount);
        gasFeeAmount.setText(
                String.format(getResources().getString(R.string.crypto_wallet_gas_fee_amount),
                        String.format(Locale.getDefault(), "%.8f",
                                Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice))));
        String valueAsset = mTxInfo.txData.baseData.value;
        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER && mTxInfo.txArgs.length > 1) {
            valueAsset = mTxInfo.txArgs[1];
        }
        TextView totalAmount = view.findViewById(R.id.total_amount);
        totalAmount.setText(String.format(
                getResources().getString(R.string.crypto_wallet_total_amount),
                String.format(Locale.getDefault(), "%.8f", Utils.fromHexWei(valueAsset)), mAsset,
                String.format(Locale.getDefault(), "%.8f",
                        Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice))));
        AssetRatioController assetRatioController = getAssetRatioController();
        if (assetRatioController != null) {
            String[] assets = {"eth"};
            String[] toCurr = {"usd"};
            assetRatioController.getPrice(
                    assets, toCurr, AssetPriceTimeframe.LIVE, (success, values) -> {
                        if (!success || values.length == 0) {
                            return;
                        }
                        double value = Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice);
                        double price = Double.valueOf(values[0].price);
                        double totalPrice = value * price;
                        TextView gasFeeAmountFiat = view.findViewById(R.id.gas_fee_amount_fiat);
                        gasFeeAmountFiat.setText(String.format(
                                getResources().getString(R.string.crypto_wallet_amount_fiat),
                                String.format(Locale.getDefault(), "%.2f", totalPrice)));
                        double totalAmountPlusGas = totalPrice + mTotalPrice;
                        TextView totalAmountFiat = view.findViewById(R.id.total_amount_fiat);
                        totalAmountFiat.setText(String.format(
                                getResources().getString(R.string.crypto_wallet_amount_fiat),
                                String.format(Locale.getDefault(), "%.2f", totalAmountPlusGas)));
                    });
        }
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }
}
