/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

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
        View view = inflater.inflate(R.layout.fragment_transaction, container, false);

        TextView gasFeeAmount = view.findViewById(R.id.gas_fee_amount);
        gasFeeAmount.setText(
                String.format("%.8f", Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice)));
        TextView totalAmount = view.findViewById(R.id.total_amount);
        totalAmount.setText(String.format("%.8f", Utils.fromHexWei(mTxInfo.txData.baseData.value))
                + " " + mAsset + " + "
                + String.format("%.8f", Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice))
                + " ETH");
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
                        gasFeeAmountFiat.setText("$" + String.format("%.2f", totalPrice));
                        double totalAmountPlusGas = totalPrice + mTotalPrice;
                        TextView totalAmountFiat = view.findViewById(R.id.total_amount_fiat);
                        totalAmountFiat.setText("$" + String.format("%.2f", totalAmountPlusGas));
                    });
        }

        return view;
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
