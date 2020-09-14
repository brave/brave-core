/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.view.MotionEvent;
import android.text.TextUtils;

import androidx.fragment.app.Fragment;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;

import java.util.List;
import org.json.JSONException;

import org.chromium.chrome.R;

/**
 * A simple {@link Fragment} subclass.
 */
public class BinanceDepositFragment extends Fragment {
	private BinanceNativeWorker mBinanceNativeWorker;
	private BinanceDepositAdapter mBinanceDepositAdapter;

	private RecyclerView currencyRecyclerView;

	private CoinNetworkModel selectedCoinNetworkModel;

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
		// Inflate the layout for this fragment
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
		currencyRecyclerView = view.findViewById(R.id.recyclerview_currency_list);
		currencyRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
		currencyRecyclerView.setOnTouchListener(new RecyclerView.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                int action = event.getAction();
                switch (action) {
                    case MotionEvent.ACTION_DOWN:
                        // Disallow NestedScrollView to intercept touch events.
                        v.getParent().requestDisallowInterceptTouchEvent(true);
                        break;

                    case MotionEvent.ACTION_UP:
                        // Allow NestedScrollView to intercept touch events.
                        v.getParent().requestDisallowInterceptTouchEvent(false);
                        break;
                }

                // Handle RecyclerView touch events.
                v.onTouchEvent(event);
                return true;
            }
        });
		mBinanceDepositAdapter = new BinanceDepositAdapter();
		currencyRecyclerView.setAdapter(mBinanceDepositAdapter);
		mBinanceDepositAdapter.setBinanceDepositListener(binanceDepositListener);

		mBinanceNativeWorker.getCoinNetworks();
		// Log.e("NTP", "Set CoinNetworks");
	}

	private BinanceDepositAdapter.BinanceDepositListener binanceDepositListener =
	new BinanceDepositAdapter.BinanceDepositListener() {
		@Override
		public void onItemClick(CoinNetworkModel coinNetworkModel) {
			selectedCoinNetworkModel = coinNetworkModel;
			mBinanceNativeWorker.getDepositInfo(coinNetworkModel.getCoin(), coinNetworkModel.getTickerNetwork());
		}
	};

	private BinanceObserver mBinanaceObserver = new BinanceObserver() {
		@Override
		public void OnGetAccessToken(boolean isSuccess) {};

		@Override
		public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {
			// Log.e("NTP", "AccountBalances : " + jsonBalances);
		};

		@Override
		public void OnGetConvertQuote(String quoteId, String quotePrice, String totalFee, String totalAmount) {};

		@Override
		public void OnGetCoinNetworks(String jsonNetworks) {
			// Log.e("NTP", "OnGetCoinNetworks"+jsonNetworks);
			try {
				BinanceCoinNetworks binanceCoinNetworks = new BinanceCoinNetworks(jsonNetworks);
				mBinanceDepositAdapter.setCurrencyList(binanceCoinNetworks.getCoinNetworksList());
				// Log.e("NTP", "CoinNetworks : " + binanceCoinNetworks.toString());
			} catch (JSONException e) {
				Log.e("NTP", e.getMessage());
			}
		};

		@Override
		public void OnGetDepositInfo(String depositAddress, String depositTag, boolean isSuccess) {
			if (isSuccess
			        && selectedCoinNetworkModel != null
			        && getView() != null) {
				FrameLayout depositLayout = (FrameLayout) getView().findViewById(R.id.binance_deposit_layout);
				ImageView depositBack = depositLayout.findViewById(R.id.currency_back);
				TextView currencyTitleText = depositLayout.findViewById(R.id.currency_text);
				ImageView deposiIcon = depositLayout.findViewById(R.id.currency_image);
				ImageView deposiQrIcon = depositLayout.findViewById(R.id.currency_qr_image);
				TextView currencyAddressText = depositLayout.findViewById(R.id.currency_address_text);
				TextView currencyMemoText = depositLayout.findViewById(R.id.currency_memo_text);
				TextView currencyAddressValueText = depositLayout.findViewById(R.id.currency_address_value_text);
				TextView currencyMemoValueText = depositLayout.findViewById(R.id.currency_memo_value_text);

				deposiQrIcon.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						QRCodeShareDialogFragment mQRCodeShareDialogFragment =
						    new QRCodeShareDialogFragment();
						mQRCodeShareDialogFragment.setQRCodeText(depositAddress);
						mQRCodeShareDialogFragment.show(
						    getActivity().getSupportFragmentManager(),
						    "QRCodeShareDialogFragment");
					}
				});

				if (selectedCoinNetworkModel != null) {
					currencyTitleText.setText(selectedCoinNetworkModel.getCoin() + (TextUtils.isEmpty(selectedCoinNetworkModel.getCoinDesc()) ? "" : " (" + selectedCoinNetworkModel.getCoinDesc() + ")"));
					currencyAddressText.setText(String.format(getResources().getString(R.string.currency_address_text), selectedCoinNetworkModel.getCoin()));
					currencyMemoText.setText(String.format(getResources().getString(R.string.currency_memo_text), selectedCoinNetworkModel.getCoin()));
					deposiIcon.setImageResource(selectedCoinNetworkModel.getCoinRes());
				}

				depositBack.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						if (currencyRecyclerView != null) {
							currencyRecyclerView.setVisibility(View.VISIBLE);
							depositLayout.setVisibility(View.GONE);
							currencyMemoText.setVisibility(View.GONE);
							currencyMemoValueText.setVisibility(View.GONE);
						}
					}
				});
				depositLayout.setVisibility(View.VISIBLE);
				if (currencyRecyclerView != null) {
					currencyRecyclerView.setVisibility(View.GONE);
				}

				currencyAddressValueText.setText(depositAddress);
				if (!TextUtils.isEmpty(depositTag)) {
					currencyMemoText.setVisibility(View.VISIBLE);
					currencyMemoValueText.setVisibility(View.VISIBLE);
					currencyMemoValueText.setText(depositTag);
				}

				selectedCoinNetworkModel = null;

				Log.e("NTP", "depositAddress : " + depositAddress);
				Log.e("NTP", "depositeTag : " + depositTag);
			}
		};

		@Override
		public void OnConfirmConvert(boolean isSuccess, String message) {};

		@Override
		public void OnGetConvertAssets(String jsonAssets) {};

		@Override
		public void OnRevokeToken(boolean isSuccess) {};
	};
}