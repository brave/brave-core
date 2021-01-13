/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.os.Bundle;
import android.text.TextUtils;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceSpinnerAdapter;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class BinanceBuyFragment extends Fragment {
    private BinanceNativeWorker mBinanceNativeWorker;

    private Button buyButton;

    private Spinner fiatSpinner;
    private Spinner cryptoSpinner;

    private String selectedFiat;
    private String selectedCrypto;

    private List<String> fiatList = new ArrayList<String>();
    private List<String> cryptoList = new ArrayList<String>();

    private static final String US = ".us";
    private static final String COM = ".com";

    public BinanceBuyFragment() {
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
        return inflater.inflate(R.layout.fragment_binance_buy, container, false);
    }

    @Override
    public void onDestroyView() {
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        super.onDestroyView();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        LinearLayout buyMainLayout = view.findViewById(R.id.binance_buy_main_layout);
        TextView noConnectionText = view.findViewById(R.id.no_connection_text);
        if (InternetConnection.isNetworkAvailable(getActivity())) {
            noConnectionText.setVisibility(View.GONE);
            buyMainLayout.setVisibility(View.VISIBLE);
            RadioGroup buyRadioGroup = view.findViewById(R.id.buy_radio_group);
            RadioButton usRadioButton = view.findViewById(R.id.us_radio);
            usRadioButton.setText(US);
            RadioButton comRadioButton = view.findViewById(R.id.com_radio);
            comRadioButton.setText(COM);
            EditText amountEditText = view.findViewById(R.id.amount_edittext);

            buyButton = view.findViewById(R.id.btn_buy);
            buyButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    String buyUrl = "";
                    if (buyRadioGroup.getCheckedRadioButtonId() == R.id.com_radio) {
                        buyUrl = String.format(BinanceWidgetManager.BINANCE_COM, selectedFiat,
                                selectedCrypto, amountEditText.getText());
                    } else if (buyRadioGroup.getCheckedRadioButtonId() == R.id.us_radio) {
                        buyUrl = String.format(BinanceWidgetManager.BINANCE_US, selectedCrypto,
                                amountEditText.getText());
                    }
                    if (!TextUtils.isEmpty(buyUrl)) {
                        TabUtils.openUrlInSameTab(buyUrl);
                    }
                    dismissBinanceBottomSheet();
                }
            });

            fiatSpinner = (Spinner) view.findViewById(R.id.fiat_spinner);
            fiatSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parentView, View selectedItemView, int position, long id) {
                    selectedFiat = fiatList.get(position);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parentView) {}
            });
            setFiatSpinner(BinanceWidgetManager.fiatList);

            cryptoSpinner = (Spinner) view.findViewById(R.id.crypto_spinner);
            cryptoSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parentView, View selectedItemView, int position, long id) {
                    selectedCrypto = cryptoList.get(position);
                    if (buyButton != null) {
                        buyButton.setText(String.format(getActivity().getResources().getString(
                                                                R.string.buy_crypto_button_text),
                                selectedCrypto));
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> parentView) {}
            });

            buyRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    if (R.id.com_radio == checkedId) {
                        setFiatSpinner(BinanceWidgetManager.fiatList);
                    } else {
                        setFiatSpinner(new ArrayList<String>(
                                Arrays.asList(BinanceWidgetManager.fiatList.get(0))));
                    }
                }
            });

            mBinanceNativeWorker.getCoinNetworks();
        } else {
            noConnectionText.setVisibility(View.VISIBLE);
            buyMainLayout.setVisibility(View.GONE);
        }
    }

    private void setFiatSpinner(List<String> fiatSpinnerList) {
        fiatList = fiatSpinnerList;

        List<CoinNetworkModel> tempFiatList = new ArrayList<CoinNetworkModel>();
        for (String fiat : fiatList) {
            tempFiatList.add(new CoinNetworkModel(fiat, "", 0));
        }

        BinanceSpinnerAdapter binanceSpinnerAdapter =
                new BinanceSpinnerAdapter(getActivity(), tempFiatList, false);
        fiatSpinner.setAdapter(binanceSpinnerAdapter);

        selectedFiat = fiatList.get(0);
    }

    private void dismissBinanceBottomSheet() {
        FragmentManager fm = getParentFragmentManager();
        CryptoWidgetBottomSheetDialogFragment fragment =
                (CryptoWidgetBottomSheetDialogFragment) fm.findFragmentByTag(
                        CryptoWidgetBottomSheetDialogFragment.TAG_FRAGMENT);
        FragmentTransaction transaction = fm.beginTransaction();

        if (fragment != null) {
            fragment.dismiss();
        }
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
                List<String> tempCryptoList = new ArrayList<String>();
                for (CoinNetworkModel coinNetworkModel :
                        binanceCoinNetworks.getCoinNetworksList()) {
                    tempCryptoList.add(coinNetworkModel.getCoin());
                }
                if (cryptoSpinner != null) {
                    cryptoList = tempCryptoList;

                    BinanceSpinnerAdapter binanceSpinnerAdapter = new BinanceSpinnerAdapter(
                            getActivity(), binanceCoinNetworks.getCoinNetworksList(), true);
                    cryptoSpinner.setAdapter(binanceSpinnerAdapter);
                    selectedCrypto = cryptoList.get(0);
                    buyButton.setText(String.format(
                            getActivity().getResources().getString(R.string.buy_crypto_button_text),
                            selectedCrypto));
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