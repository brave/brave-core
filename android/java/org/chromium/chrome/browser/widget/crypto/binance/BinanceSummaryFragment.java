/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.Context;
import android.os.Bundle;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;

import java.util.List;

public class BinanceSummaryFragment extends Fragment {
    private BinanceNativeWorker mBinanceNativeWorker;
    private LinearLayout summaryLayout;

    private static final String ZERO_BALANCE = "0.00000000";
    private static final String ZERO_USD_BALANCE = "0.00";
    private static final String BTC = "BTC";

    public BinanceSummaryFragment() {
        // Required empty public constructor
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBinanceNativeWorker = BinanceNativeWorker.getInstance();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mBinanceNativeWorker.AddObserver(mBinanaceObserver);
        return inflater.inflate(R.layout.fragment_binance_summary, container, false);
    }

    @Override
    public void onDestroyView() {
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        super.onDestroyView();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        TextView binanceBalanceText = view.findViewById(R.id.binance_balance_text);
        TextView binanceBtcText = view.findViewById(R.id.binance_btc_text);
        TextView binanceUSDBalanceText = view.findViewById(R.id.binance_usd_balance_text);

        if (BinanceWidgetManager.binanceAccountBalance != null) {
            binanceBalanceText.setText(
                    String.valueOf(BinanceWidgetManager.binanceAccountBalance.getTotalBTC()));
            binanceBtcText.setText(BTC);
            binanceUSDBalanceText.setText(String.format(
                    getActivity().getResources().getString(R.string.usd_balance),
                    String.valueOf(BinanceWidgetManager.binanceAccountBalance.getTotalUSD())));
        }
        summaryLayout = view.findViewById(R.id.summary_layout);
        mBinanceNativeWorker.getCoinNetworks();
    }

    private BinanceObserver mBinanaceObserver = new BinanceObserver() {
        @Override
        public void OnGetAccessToken(boolean isSuccess){};

        @Override
        public void OnGetAccountBalances(String jsonBalances, boolean isSuccess){};

        @Override
        public void OnGetConvertQuote(
                String quoteId, String quotePrice, String totalFee, String totalAmount){};

        @Override
        public void OnGetCoinNetworks(String jsonNetworks) {
            try {
                BinanceCoinNetworks binanceCoinNetworks = new BinanceCoinNetworks(jsonNetworks);
                LayoutInflater inflater = (LayoutInflater) getActivity().getSystemService(
                        Context.LAYOUT_INFLATER_SERVICE);
                if (summaryLayout != null) {
                    summaryLayout.removeAllViews();
                }
                for (CoinNetworkModel coinNetworkModel :
                        binanceCoinNetworks.getCoinNetworksList()) {
                    final View view = inflater.inflate(R.layout.binance_summary_item, null);

                    ImageView currencyImageView = view.findViewById(R.id.currency_image);
                    TextView currencyText = view.findViewById(R.id.currency_text);
                    TextView currencyValueText = view.findViewById(R.id.currency_value_text);
                    TextView currencyUsdText = view.findViewById(R.id.currency_usd_text);

                    currencyText.setText(coinNetworkModel.getCoin());

                    if (coinNetworkModel.getCoinRes() == 0) {
                        currencyImageView.setImageResource(R.drawable.eth);
                        currencyImageView.setVisibility(View.INVISIBLE);
                    } else {
                        currencyImageView.setImageResource(coinNetworkModel.getCoinRes());
                    }

                    String coinBalance;
                    String usdBalance;
                    if (BinanceWidgetManager.binanceAccountBalance.getCurrencyValue(
                                coinNetworkModel.getCoin())
                            != null) {
                        Pair<Double, Double> currencyValue =
                                BinanceWidgetManager.binanceAccountBalance.getCurrencyValue(
                                        coinNetworkModel.getCoin());
                        coinBalance = String.valueOf(currencyValue.first);
                        usdBalance = String.valueOf(currencyValue.second);
                    } else {
                        coinBalance = ZERO_BALANCE;
                        usdBalance = ZERO_USD_BALANCE;
                    }

                    currencyValueText.setText(coinBalance);
                    currencyUsdText.setText(String.format(
                            getActivity().getResources().getString(R.string.usd_balance),
                            usdBalance));

                    if (summaryLayout != null) {
                        summaryLayout.addView(view);
                    }
                }
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
        };

        @Override
        public void OnGetDepositInfo(
                String depositAddress, String depositeTag, boolean isSuccess){};

        @Override
        public void OnConfirmConvert(boolean isSuccess, String message){};

        @Override
        public void OnGetConvertAssets(String jsonAssets){};

        @Override
        public void OnRevokeToken(boolean isSuccess){};
    };
}