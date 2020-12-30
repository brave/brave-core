/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;
import org.chromium.ui.widget.Toast;

import java.util.List;

public class BinanceDepositFragment extends Fragment {
    private BinanceNativeWorker mBinanceNativeWorker;

    private CoinNetworkModel selectedCoinNetworkModel;
    private LinearLayout depositCoinListLayout;
    private ProgressBar binanceCoinsProgress;
    private NestedScrollView currentNestedScrollView;
    private EditText searchEditText;
    private String networksJson;

    public BinanceDepositFragment() {
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
        return inflater.inflate(R.layout.fragment_binance_deposit, container, false);
    }

    @Override
    public void onDestroyView() {
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        super.onDestroyView();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        LinearLayout depositMainLayout = view.findViewById(R.id.binance_deposit_main_layout);
        TextView noConnectionText = view.findViewById(R.id.no_connection_text);
        if (InternetConnection.isNetworkAvailable(getActivity())) {
            noConnectionText.setVisibility(View.GONE);
            depositMainLayout.setVisibility(View.VISIBLE);
            depositCoinListLayout = view.findViewById(R.id.deposit_layout);
            binanceCoinsProgress = view.findViewById(R.id.binance_coins_progress);
            searchEditText = view.findViewById(R.id.binance_coin_search);
            searchEditText.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

                @Override
                public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                    if (networksJson != null && !TextUtils.isEmpty(networksJson)) {
                        showCoinList(networksJson, charSequence.toString().toLowerCase());
                    }
                }

                @Override
                public void afterTextChanged(Editable editable) {}
            });
            depositCoinListLayout.setVisibility(View.GONE);
            binanceCoinsProgress.setVisibility(View.VISIBLE);
            mBinanceNativeWorker.getCoinNetworks();
        } else {
            noConnectionText.setVisibility(View.VISIBLE);
            depositMainLayout.setVisibility(View.GONE);
        }
    }

    private void showCoinList(String networks, String filterQuery) {
        try {
            BinanceCoinNetworks binanceCoinNetworks = new BinanceCoinNetworks(networks);
            LayoutInflater inflater = (LayoutInflater) getActivity().getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            if (depositCoinListLayout != null) {
                depositCoinListLayout.removeAllViews();
            }
            for (CoinNetworkModel coinNetworkModel : binanceCoinNetworks.getCoinNetworksList()) {
                if (filterQuery == null
                        || (coinNetworkModel.getCoin().toLowerCase().contains(filterQuery)
                                || coinNetworkModel.getCoinDesc().toLowerCase().contains(
                                        filterQuery))) {
                    final View view = inflater.inflate(R.layout.binance_deposit_item, null);

                    ImageView currencyImageView = view.findViewById(R.id.currency_image);
                    TextView currencyText = view.findViewById(R.id.currency_text);

                    currencyText.setText(coinNetworkModel.getCoin()
                            + (TextUtils.isEmpty(coinNetworkModel.getCoinDesc())
                                            ? ""
                                            : " (" + coinNetworkModel.getCoinDesc() + ")"));
                    if (coinNetworkModel.getCoinRes() == 0) {
                        currencyImageView.setImageResource(R.drawable.eth);
                        currencyImageView.setVisibility(View.INVISIBLE);
                    } else {
                        currencyImageView.setImageResource(coinNetworkModel.getCoinRes());
                    }
                    view.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            selectedCoinNetworkModel = coinNetworkModel;
                            mBinanceNativeWorker.getDepositInfo(coinNetworkModel.getCoin(),
                                    coinNetworkModel.getTickerNetwork());
                        }
                    });
                    if (depositCoinListLayout != null) {
                        depositCoinListLayout.addView(view);
                    }
                }
            }
            if (depositCoinListLayout != null) {
                depositCoinListLayout.setVisibility(View.VISIBLE);
            }
            if (binanceCoinsProgress != null) {
                binanceCoinsProgress.setVisibility(View.GONE);
            }
        } catch (JSONException e) {
            Log.e("NTP", e.getMessage());
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
            networksJson = jsonNetworks;
            showCoinList(jsonNetworks, null);
        };

        @Override
        public void OnGetDepositInfo(String depositAddress, String depositTag, boolean isSuccess) {
            if (isSuccess && selectedCoinNetworkModel != null && getView() != null) {
                FrameLayout depositLayout =
                        (FrameLayout) getView().findViewById(R.id.binance_deposit_layout);
                ImageView depositBack = depositLayout.findViewById(R.id.currency_back);
                TextView currencyTitleText = depositLayout.findViewById(R.id.currency_text);
                ImageView depositIcon = depositLayout.findViewById(R.id.currency_image);
                ImageView depositQrIcon = depositLayout.findViewById(R.id.currency_qr_image);
                TextView currencyAddressText =
                        depositLayout.findViewById(R.id.currency_address_text);
                TextView currencyMemoText = depositLayout.findViewById(R.id.currency_memo_text);
                TextView currencyAddressValueText =
                        depositLayout.findViewById(R.id.currency_address_value_text);
                TextView currencyMemoValueText =
                        depositLayout.findViewById(R.id.currency_memo_value_text);
                Button btnCopyAddress = depositLayout.findViewById(R.id.btn_copy_address);
                Button btnCopyMemo = depositLayout.findViewById(R.id.btn_copy_memo);

                depositQrIcon.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        QRCodeShareDialogFragment mQRCodeShareDialogFragment =
                                new QRCodeShareDialogFragment();
                        mQRCodeShareDialogFragment.setQRCodeText(depositAddress);
                        mQRCodeShareDialogFragment.show(
                                getParentFragmentManager(), "QRCodeShareDialogFragment");
                    }
                });

                if (selectedCoinNetworkModel != null) {
                    currencyTitleText.setText(selectedCoinNetworkModel.getCoin()
                            + (TextUtils.isEmpty(selectedCoinNetworkModel.getCoinDesc())
                                            ? ""
                                            : " (" + selectedCoinNetworkModel.getCoinDesc() + ")"));
                    currencyAddressText.setText(String.format(
                            getActivity().getResources().getString(R.string.currency_address_text),
                            selectedCoinNetworkModel.getCoin()));
                    currencyMemoText.setText(String.format(
                            getActivity().getResources().getString(R.string.currency_memo_text),
                            selectedCoinNetworkModel.getCoin()));
                    depositIcon.setImageResource(selectedCoinNetworkModel.getCoinRes());
                }

                depositBack.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (depositCoinListLayout != null) {
                            depositCoinListLayout.setVisibility(View.VISIBLE);
                            depositLayout.setVisibility(View.GONE);
                            currencyMemoText.setVisibility(View.GONE);
                            currencyMemoValueText.setVisibility(View.GONE);
                            btnCopyMemo.setVisibility(View.GONE);
                        }
                        if (searchEditText != null) {
                            searchEditText.setVisibility(View.VISIBLE);
                        }
                        selectedCoinNetworkModel = null;
                    }
                });
                depositLayout.setVisibility(View.VISIBLE);
                if (depositCoinListLayout != null) {
                    depositCoinListLayout.setVisibility(View.GONE);
                }

                if (searchEditText != null) {
                    searchEditText.setVisibility(View.GONE);
                }

                currencyAddressValueText.setText(depositAddress);
                if (!TextUtils.isEmpty(depositTag)) {
                    currencyMemoText.setVisibility(View.VISIBLE);
                    currencyMemoValueText.setVisibility(View.VISIBLE);
                    btnCopyMemo.setVisibility(View.VISIBLE);
                    currencyMemoValueText.setText(depositTag);
                }

                btnCopyAddress.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (selectedCoinNetworkModel != null) {
                            copyTextToClipboard(
                                    String.format(getActivity().getResources().getString(
                                                          R.string.currency_address_text),
                                            selectedCoinNetworkModel.getCoin()),
                                    depositAddress);
                        }
                    }
                });

                btnCopyMemo.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (selectedCoinNetworkModel != null) {
                            copyTextToClipboard(
                                    String.format(getActivity().getResources().getString(
                                                          R.string.currency_memo_text),
                                            selectedCoinNetworkModel.getCoin()),
                                    depositTag);
                        }
                    }
                });
            }
        };

        @Override
        public void OnConfirmConvert(boolean isSuccess, String message){};

        @Override
        public void OnGetConvertAssets(String jsonAssets){};

        @Override
        public void OnRevokeToken(boolean isSuccess){};
    };

    private void copyTextToClipboard(String title, String textToCopy) {
        android.content.ClipboardManager clipboard =
                (android.content.ClipboardManager) getActivity().getSystemService(
                        Context.CLIPBOARD_SERVICE);
        android.content.ClipData clip = android.content.ClipData.newPlainText(title, textToCopy);
        clipboard.setPrimaryClip(clip);
        Toast.makeText(getActivity(), R.string.text_has_been_copied, Toast.LENGTH_LONG).show();
    }
}