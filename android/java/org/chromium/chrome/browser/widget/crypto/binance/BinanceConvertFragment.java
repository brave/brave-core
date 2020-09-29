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
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;
import org.chromium.chrome.browser.widget.crypto.binance.ConvertAsset;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * A simple {@link Fragment} subclass.
 */
public class BinanceConvertFragment extends Fragment {
    private BinanceNativeWorker mBinanceNativeWorker;
    private CurrencyListAdapter mCurrencyListAdapter;

    private Button convertButton;

    private Spinner cryptoSpinner1;
    private Spinner cryptoSpinner2;

    private String selectedCrypto1;
    private ConvertAsset selectedCrypto2;

    private String[] cryptoArray1;
    private ConvertAsset[] cryptoArray2;

    private BinanceConvert binanceConvert;

    private TextView binanceConvertTitle;
    private LinearLayout convertLayout;
    private LinearLayout errorLayout;

    private LinearLayout confirmLayout;
    private TextView convertCurrencyText;
    private TextView convertFeeText;
    private TextView convertBalanceText;

    private Button confirmButton;

    public BinanceConvertFragment() {
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
        // Inflate the layout for this fragment
        mBinanceNativeWorker.AddObserver(mBinanaceObserver);
        return inflater.inflate(R.layout.fragment_binance_convert, container, false);
    }

    @Override
    public void onDestroyView() {
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        super.onDestroyView();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        EditText amountEditText = view.findViewById(R.id.amount_edittext);
        binanceConvertTitle = view.findViewById(R.id.binance_convert_title);

        convertLayout = view.findViewById(R.id.convert_layout);
        errorLayout = view.findViewById(R.id.error_layout);
        TextView errorText = view.findViewById(R.id.error_message_text);

        Button retryButton = view.findViewById(R.id.btn_retry);
        retryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                convertLayout.setVisibility(View.VISIBLE);
                errorLayout.setVisibility(View.GONE);
            }
        });

        confirmLayout = view.findViewById(R.id.confirm_convert_layout);
        convertCurrencyText = view.findViewById(R.id.convert_currency_text);
        convertFeeText = view.findViewById(R.id.convert_fee_text);
        convertBalanceText = view.findViewById(R.id.convert_balance_text);
        confirmButton = view.findViewById(R.id.btn_confirm);

        Button cancelButton = view.findViewById(R.id.btn_cancel);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                convertLayout.setVisibility(View.VISIBLE);
                confirmLayout.setVisibility(View.GONE);
            }
        });

        convertButton = view.findViewById(R.id.btn_convert);
        convertButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                double convertAmount = !TextUtils.isEmpty(amountEditText.getText().toString())
                        ? Double.valueOf(amountEditText.getText().toString())
                        : 0.0;
                BinanceAccountBalance binanceAccountBalance =
                        BinanceWidgetManager.getInstance().getBinanceAccountBalance();
                double availableBalance =
                        binanceAccountBalance.getCurrencyValue(selectedCrypto1) != null
                        ? Double.valueOf(
                                binanceAccountBalance.getCurrencyValue(selectedCrypto1).first)
                        : 0.0;
                if (availableBalance < convertAmount) {
                    convertLayout.setVisibility(View.GONE);
                    errorLayout.setVisibility(View.VISIBLE);
                    errorText.setText("Not enough balance for conversion");
                } else if (Double.valueOf(selectedCrypto2.getMinAmount()) > convertAmount) {
                    convertLayout.setVisibility(View.GONE);
                    errorLayout.setVisibility(View.VISIBLE);
                    errorText.setText("The minimum amount to convert is : "
                            + selectedCrypto2.getMinAmount() + " " + selectedCrypto1);
                } else {
                    mBinanceNativeWorker.getConvertQuote(selectedCrypto1,
                            selectedCrypto2.getAsset(), String.valueOf(convertAmount));
                }
            }
        });

        cryptoSpinner1 = (Spinner) view.findViewById(R.id.crypto_spinner_1);
        cryptoSpinner1.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(
                    AdapterView<?> parentView, View selectedItemView, int position, long id) {
                selectedCrypto1 = cryptoArray1[position];
                setCryptoSpinner(selectedCrypto1);
                setTitle();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
            }
        });

        cryptoSpinner2 = (Spinner) view.findViewById(R.id.crypto_spinner_2);
        cryptoSpinner2.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(
                    AdapterView<?> parentView, View selectedItemView, int position, long id) {
                selectedCrypto2 = cryptoArray2[position];
            }

            @Override
            public void onNothingSelected(AdapterView<?> parentView) {
                // your code here
            }
        });

        mBinanceNativeWorker.getConvertAssets();
    }

    private void dismissBinanceBottomSheet() {
        FragmentManager fm = getActivity().getSupportFragmentManager();
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
                String quoteId, String quotePrice, String totalFee, String totalAmount) {
            convertLayout.setVisibility(View.GONE);
            confirmLayout.setVisibility(View.VISIBLE);
            convertCurrencyText.setText(quotePrice + selectedCrypto1);
            convertFeeText.setText(totalFee + selectedCrypto1);
            convertBalanceText.setText(totalAmount + selectedCrypto2.getAsset());

            confirmButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mBinanceNativeWorker.confirmConvert(quoteId);
                }
            });
        };

        @Override
        public void OnGetCoinNetworks(String jsonNetworks){};

        @Override
        public void OnGetDepositInfo(
                String depositAddress, String depositeTag, boolean isSuccess){};

        @Override
        public void OnConfirmConvert(boolean isSuccess, String message) {
            if (isSuccess) {
                convertLayout.setVisibility(View.VISIBLE);
                confirmLayout.setVisibility(View.GONE);
            }
        };

        @Override
        public void OnGetConvertAssets(String jsonAssets) {
            try {
                binanceConvert = new BinanceConvert(jsonAssets);
                cryptoArray1 = (String[]) binanceConvert.getCurrencyKeys().toArray(
                        new String[binanceConvert.getCurrencyKeys().size()]);
                ArrayAdapter<String> cryptoAdapter1 = new ArrayAdapter<String>(
                        getActivity(), R.layout.binance_spinner_text_layout, cryptoArray1);
                cryptoAdapter1.setDropDownViewResource(android.R.layout.simple_list_item_1);
                cryptoSpinner1.setAdapter(cryptoAdapter1);

                selectedCrypto1 = cryptoArray1[0];
                setCryptoSpinner(selectedCrypto1);
                setTitle();
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
        };

        @Override
        public void OnRevokeToken(boolean isSuccess){};
    };

    private void setCryptoSpinner(String key) {
        List<ConvertAsset> convertCryptoList = binanceConvert.getCurrencyValue(key);
        if (cryptoSpinner2 != null) {
            cryptoArray2 = (ConvertAsset[]) convertCryptoList.toArray(new ConvertAsset[0]);
            List<String> tempCryptoList = new ArrayList<>();
            for (ConvertAsset convertAsset : convertCryptoList) {
                tempCryptoList.add(convertAsset.getAsset());
            }
            ArrayAdapter<String> cryptoAdapter2 =
                    new ArrayAdapter<String>(getActivity(), R.layout.binance_spinner_text_layout,
                            (String[]) tempCryptoList.toArray(new String[tempCryptoList.size()]));
            cryptoAdapter2.setDropDownViewResource(android.R.layout.simple_list_item_1);
            cryptoSpinner2.setAdapter(cryptoAdapter2);
            selectedCrypto2 = cryptoArray2[0];
        }
    }

    private void setTitle() {
        if (binanceConvertTitle != null) {
            BinanceAccountBalance binanceAccountBalance =
                    BinanceWidgetManager.getInstance().getBinanceAccountBalance();
            binanceConvertTitle.setText(String.format(
                    getResources().getString(R.string.available_balance_text),
                    binanceAccountBalance.getCurrencyValue(selectedCrypto1) != null
                            ? String.valueOf(
                                    binanceAccountBalance.getCurrencyValue(selectedCrypto1).first)
                            : "0.000000",
                    selectedCrypto1));
        }
    }
}