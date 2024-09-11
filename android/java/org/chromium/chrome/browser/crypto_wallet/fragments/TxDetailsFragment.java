/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class TxDetailsFragment extends Fragment {
    private TransactionInfo mTxInfo;
    private TxData1559 mTxData1559;

    public static TxDetailsFragment newInstance(TransactionInfo txInfo) {
        return new TxDetailsFragment(txInfo);
    }

    private TxDetailsFragment(TransactionInfo txInfo) {
        mTxInfo = txInfo;
        mTxData1559 =
                mTxInfo.txDataUnion.which() == TxDataUnion.Tag.EthTxData1559
                        ? mTxInfo.txDataUnion.getEthTxData1559()
                        : null;
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_transaction_details, container, false);

        setupView(view);

        return view;
    }

    @SuppressLint("ClickableViewAccessibility")
    public void setupView(View view) {
        if ((mTxInfo.txParams.length == 0 || mTxInfo.txArgs.length == 0)
                && mTxData1559 != null
                && mTxData1559.baseData.data.length == 0) {
            return;
        }
        assert mTxInfo.txParams.length == mTxInfo.txArgs.length;
        String functionType = getString(TransactionUtils.getTxType(mTxInfo));

        TextView functionTypeWidget = view.findViewById(R.id.function_type);
        functionTypeWidget.setText(
                String.format(
                        getResources().getString(R.string.wallet_details_function_type),
                        functionType));

        if (mTxInfo.txParams.length > 0) {
            String detailsParam1 = mTxInfo.txParams[0] + ": " + mTxInfo.txArgs[0];
            TextView detailsParam1Widget = view.findViewById(R.id.tx_details_param_1);
            detailsParam1Widget.setVisibility(View.VISIBLE);
            detailsParam1Widget.setText(detailsParam1);
        }

        if (mTxInfo.txParams.length > 1) {
            String detailsParam2 = mTxInfo.txParams[1] + ": " + mTxInfo.txArgs[1];
            TextView detailsParam2Widget = view.findViewById(R.id.tx_details_param_2);
            detailsParam2Widget.setVisibility(View.VISIBLE);
            detailsParam2Widget.setText(detailsParam2);
        }

        if (mTxInfo.txParams.length > 3) {
            String detailsParam3 = mTxInfo.txParams[2] + ": " + mTxInfo.txArgs[2];
            TextView detailsParam3Widget = view.findViewById(R.id.tx_details_param_3);
            detailsParam3Widget.setVisibility(View.VISIBLE);
            detailsParam3Widget.setText(detailsParam3);
        }

        if (mTxInfo.txParams.length == 0
                && mTxData1559 != null
                && mTxData1559.baseData.data.length != 0) {
            String detailsParam1 = Utils.numberArrayToHexStr(mTxData1559.baseData.data);
            TextView detailsParam1Widget = view.findViewById(R.id.tx_details_param_1);
            detailsParam1Widget.setVisibility(View.VISIBLE);
            detailsParam1Widget.setText(detailsParam1);
            detailsParam1Widget.setOnTouchListener(
                    new View.OnTouchListener() {
                        @Override
                        @SuppressLint("ClickableViewAccessibility")
                        public boolean onTouch(View v, MotionEvent event) {
                            v.getParent().requestDisallowInterceptTouchEvent(true);
                            switch (event.getAction() & MotionEvent.ACTION_MASK) {
                                case MotionEvent.ACTION_UP:
                                    v.getParent().requestDisallowInterceptTouchEvent(false);
                                    break;
                            }
                            return false;
                        }
                    });
        }
    }
}
