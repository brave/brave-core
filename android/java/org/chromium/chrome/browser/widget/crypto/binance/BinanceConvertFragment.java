/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.app.Activity;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.text.TextUtils;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
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
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;
import org.chromium.chrome.browser.widget.crypto.binance.ConvertAsset;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class BinanceConvertFragment extends Fragment {
    private BinanceNativeWorker mBinanceNativeWorker;

    private Button convertButton;

    private Spinner cryptoSpinner1;
    private Spinner cryptoSpinner2;

    private String selectedCrypto1;
    private ConvertAsset selectedCrypto2;

    private List<String> cryptoList1 = new ArrayList<String>();
    private List<ConvertAsset> cryptoList2 = new ArrayList<ConvertAsset>();

    private BinanceConvert binanceConvert;

    private TextView binanceConvertTitle;
    private LinearLayout convertLayout;
    private LinearLayout errorLayout;
    private LinearLayout successLayout;
    private EditText amountEditText;

    private LinearLayout confirmLayout;
    private TextView convertCurrencyText;
    private TextView convertFeeText;
    private TextView convertBalanceText;
    private CountDownTimer countDownTimer;
    private ProgressBar binanceWidgetProgress;
    private TextView successText;

    private double convertAmount;
    private String convertedAmount;
    private CryptoWidgetBottomSheetDialogFragment.BinanceBottomSheetListener mBinanceBottomSheetListener;

    private static final String ZERO_BALANCE = "0.000000";

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
        mBinanceNativeWorker.AddObserver(mBinanaceObserver);
        return inflater.inflate(R.layout.fragment_binance_convert, container, false);
    }

    @Override
    public void onDestroyView() {
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        cancelTimer();
        super.onDestroyView();
    }

    public void setBinanceBottomSheetListener(CryptoWidgetBottomSheetDialogFragment.BinanceBottomSheetListener binanceBottomSheetListener) {
        mBinanceBottomSheetListener = binanceBottomSheetListener;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        FrameLayout convertMainLayout = view.findViewById(R.id.binance_convert_main_layout);
        TextView noConnectionText = view.findViewById(R.id.no_connection_text);
        if (InternetConnection.isNetworkAvailable(getActivity())) {
            noConnectionText.setVisibility(View.GONE);
            convertMainLayout.setVisibility(View.VISIBLE);
            amountEditText = view.findViewById(R.id.amount_edittext);
            binanceConvertTitle = view.findViewById(R.id.binance_convert_title);
            convertLayout = view.findViewById(R.id.convert_layout);
            convertLayout.setVisibility(View.GONE);
            binanceWidgetProgress = view.findViewById(R.id.binance_widget_progress);
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

            successLayout = view.findViewById(R.id.success_layout);
            successText = view.findViewById(R.id.success_message_text);
            Button continueButton = view.findViewById(R.id.btn_continue);
            continueButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    amountEditText.getText().clear();
                    mBinanceBottomSheetListener.onContinue();
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
                    cancelTimer();
                    amountEditText.getText().clear();
                    convertLayout.setVisibility(View.VISIBLE);
                    confirmLayout.setVisibility(View.GONE);
                }
            });

            convertButton = view.findViewById(R.id.btn_convert);
            convertButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    InputMethodManager imm = (InputMethodManager) getActivity().getSystemService(
                            Activity.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(amountEditText.getWindowToken(), 0);
                    if (InternetConnection.isNetworkAvailable(getActivity())) {
                        convertAmount = !TextUtils.isEmpty(amountEditText.getText().toString())
                                ? Double.valueOf(amountEditText.getText().toString())
                                : 0.0;
                        double availableBalance =
                                BinanceWidgetManager.binanceAccountBalance.getCurrencyValue(
                                        selectedCrypto1)
                                        != null
                                ? Double.valueOf(BinanceWidgetManager.binanceAccountBalance
                                                         .getCurrencyValue(selectedCrypto1)
                                                         .first)
                                : 0.0;
                        if (availableBalance < convertAmount) {
                            convertLayout.setVisibility(View.GONE);
                            errorLayout.setVisibility(View.VISIBLE);
                            errorText.setText(getActivity().getResources().getString(
                                    R.string.not_enough_balance));
                        } else if (Double.valueOf(selectedCrypto2.getMinAmount()) > convertAmount) {
                            convertLayout.setVisibility(View.GONE);
                            errorLayout.setVisibility(View.VISIBLE);
                            errorText.setText(
                                    String.format(getActivity().getResources().getString(
                                                          R.string.minimum_amount_to_convert),
                                            selectedCrypto2.getMinAmount(), selectedCrypto1));
                        } else {
                            mBinanceNativeWorker.getConvertQuote(selectedCrypto1,
                                    selectedCrypto2.getAsset(), String.valueOf(convertAmount));
                        }
                    } else {
                        Toast.makeText(getActivity(), R.string.please_check_the_connection,
                                     Toast.LENGTH_SHORT)
                                .show();
                    }
                }
            });

            cryptoSpinner1 = (Spinner) view.findViewById(R.id.crypto_spinner_1);
            cryptoSpinner1.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parentView, View selectedItemView, int position, long id) {
                    selectedCrypto1 = cryptoList1.get(position);
                    setCryptoSpinner(selectedCrypto1);
                    setTitle();
                }

                @Override
                public void onNothingSelected(AdapterView<?> parentView) {}
            });

            cryptoSpinner2 = (Spinner) view.findViewById(R.id.crypto_spinner_2);
            cryptoSpinner2.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(
                        AdapterView<?> parentView, View selectedItemView, int position, long id) {
                    selectedCrypto2 = cryptoList2.get(position);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parentView) {}
            });

            mBinanceNativeWorker.getConvertAssets();
            binanceWidgetProgress.setVisibility(View.VISIBLE);
        } else {
            noConnectionText.setVisibility(View.VISIBLE);
            convertMainLayout.setVisibility(View.GONE);
        }
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
        public void OnGetConvertQuote(
                String quoteId, String quotePrice, String totalFee, String totalAmount) {
            convertLayout.setVisibility(View.GONE);
            confirmLayout.setVisibility(View.VISIBLE);
            convertedAmount = !TextUtils.isEmpty(totalAmount) ? String.format(
                            Locale.getDefault(), "%.6f", Double.parseDouble(totalAmount))
                                                    : ZERO_BALANCE;
            convertCurrencyText.setText(String.format(
                    getActivity().getResources().getString(R.string.convert_stat_text),
                    convertAmount != 0.0 ? String.format(
                            Locale.getDefault(), "%.6f", convertAmount)
                                                   : ZERO_BALANCE,
                    selectedCrypto1));
            convertFeeText.setText(String.format(
                    getActivity().getResources().getString(R.string.convert_stat_text),
                    !TextUtils.isEmpty(totalFee) ? String.format(
                            Locale.getDefault(), "%.6f", Double.parseDouble(totalFee))
                                                 : ZERO_BALANCE,
                    selectedCrypto1));
            convertBalanceText.setText(String.format(
                    getActivity().getResources().getString(R.string.convert_stat_text),
                    convertedAmount,
                    selectedCrypto2.getAsset()));
            startTimer();

            confirmButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mBinanceNativeWorker.confirmConvert(quoteId);
                    cancelTimer();
                }
            });
        };

        @Override
        public void OnConfirmConvert(boolean isSuccess, String message) {
            confirmLayout.setVisibility(View.GONE);
            if (!isSuccess) {
                convertLayout.setVisibility(View.VISIBLE);
                Toast.makeText(getActivity(), R.string.conversion_failed, Toast.LENGTH_SHORT)
                        .show();
            } else {
                successLayout.setVisibility(View.VISIBLE);
                if (successText != null) successText.setText(String.format(getActivity().getResources().getString(R.string.convert_success), convertAmount, selectedCrypto1,
                            convertedAmount, selectedCrypto2.getAsset()));
            }
            cancelTimer();
        };

        @Override
        public void OnGetConvertAssets(String jsonAssets) {
            try {
                binanceConvert = new BinanceConvert(jsonAssets);
                cryptoList1 = binanceConvert.getCurrencyKeys();

                List<CoinNetworkModel> tempCryptoList = new ArrayList<CoinNetworkModel>();
                for (String crypto : cryptoList1) {
                    int cryptoRes = 0;
                    if (BinanceWidgetManager.usCurrenciesMap.containsKey(crypto)) {
                        cryptoRes = BinanceWidgetManager.usCurrenciesMap.get(crypto).getCoinRes();
                    } else if (BinanceWidgetManager.comCurrenciesMap.containsKey(crypto)) {
                        cryptoRes = BinanceWidgetManager.comCurrenciesMap.get(crypto).getCoinRes();
                    } else if (BinanceWidgetManager.extraCurrenciesMap.containsKey(crypto)) {
                        cryptoRes =
                                BinanceWidgetManager.extraCurrenciesMap.get(crypto).getCoinRes();
                    }
                    tempCryptoList.add(new CoinNetworkModel(crypto, "", cryptoRes));
                }

                BinanceSpinnerAdapter binanceSpinnerAdapter =
                        new BinanceSpinnerAdapter(getActivity(), tempCryptoList, true);
                cryptoSpinner1.setAdapter(binanceSpinnerAdapter);

                selectedCrypto1 = cryptoList1.get(0);
                setCryptoSpinner(selectedCrypto1);
                setTitle();
                convertLayout.setVisibility(View.VISIBLE);
                binanceWidgetProgress.setVisibility(View.GONE);
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
        };
    };

    private void setCryptoSpinner(String key) {
        cryptoList2 = binanceConvert.getCurrencyValue(key);
        if (cryptoSpinner2 != null) {
            List<CoinNetworkModel> tempCryptoList = new ArrayList<CoinNetworkModel>();
            for (ConvertAsset convertAsset : cryptoList2) {
                int cryptoRes = 0;
                if (BinanceWidgetManager.usCurrenciesMap.containsKey(convertAsset.getAsset())) {
                    cryptoRes = BinanceWidgetManager.usCurrenciesMap.get(convertAsset.getAsset())
                                        .getCoinRes();
                } else if (BinanceWidgetManager.comCurrenciesMap.containsKey(
                                   convertAsset.getAsset())) {
                    cryptoRes = BinanceWidgetManager.comCurrenciesMap.get(convertAsset.getAsset())
                                        .getCoinRes();
                } else if (BinanceWidgetManager.extraCurrenciesMap.containsKey(
                                   convertAsset.getAsset())) {
                    cryptoRes = BinanceWidgetManager.extraCurrenciesMap.get(convertAsset.getAsset())
                                        .getCoinRes();
                }
                tempCryptoList.add(new CoinNetworkModel(convertAsset.getAsset(), "", cryptoRes));
            }

            BinanceSpinnerAdapter binanceSpinnerAdapter =
                    new BinanceSpinnerAdapter(getActivity(), tempCryptoList, true);
            cryptoSpinner2.setAdapter(binanceSpinnerAdapter);

            selectedCrypto2 = cryptoList2.get(0);
        }
    }

    private void setTitle() {
        if (binanceConvertTitle != null) {
            binanceConvertTitle.setText(String.format(
                    getActivity().getResources().getString(R.string.available_balance_text),
                    BinanceWidgetManager.binanceAccountBalance.getCurrencyValue(selectedCrypto1)
                                    != null
                            ? String.format(Locale.getDefault(), "%.6f",
                                    BinanceWidgetManager.binanceAccountBalance
                                            .getCurrencyValue(selectedCrypto1)
                                            .first)
                            : ZERO_BALANCE,
                    selectedCrypto1));
        }
    }

    // start timer function
    public void startTimer() {
        countDownTimer = new CountDownTimer(30000, 1000) {
            @Override
            public void onTick(long millisUntilFinished) {
                if (confirmButton != null) {
                    confirmButton.setText(String.format(getActivity().getResources().getString(
                                                                R.string.confirm_convert_binance),
                            String.valueOf(millisUntilFinished / 1000)));
                }
            }

            @Override
            public void onFinish() {
                cancelTimer();
                amountEditText.getText().clear();
                convertLayout.setVisibility(View.VISIBLE);
                confirmLayout.setVisibility(View.GONE);
            }
        };
        countDownTimer.start();
    }

    // cancel timer
    public void cancelTimer() {
        if (countDownTimer != null) countDownTimer.cancel();
    }
}