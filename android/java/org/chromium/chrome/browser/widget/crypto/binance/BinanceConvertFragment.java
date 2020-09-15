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
import android.widget.EditText;
import android.util.Pair;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.AdapterView;
import android.text.TextUtils;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

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
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.List;
import java.util.ArrayList;
import org.json.JSONException;

import org.chromium.chrome.R;

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
	private String selectedCrypto2;

	private String[] cryptoArray1;
	private String[] cryptoArray2;

	private BinanceConvert binanceConvert;

	private TextView binanceConvertTitle;

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

		convertButton = view.findViewById(R.id.btn_convert);
		convertButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {

				dismissBinanceBottomSheet();
			}
		});

		cryptoSpinner1 = (Spinner)view.findViewById(R.id.crypto_spinner_1);
		cryptoSpinner1.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				selectedCrypto1 = cryptoArray1[position];
				setCryptoSpinner(selectedCrypto1);
				setTitle();
			}

			@Override
			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here
			}

		});

		cryptoSpinner2 = (Spinner)view.findViewById(R.id.crypto_spinner_2);
		cryptoSpinner2.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
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
		CryptoWidgetBottomSheetDialogFragment fragment = (CryptoWidgetBottomSheetDialogFragment) fm
		        .findFragmentByTag(CryptoWidgetBottomSheetDialogFragment.TAG_FRAGMENT);
		FragmentTransaction transaction = fm.beginTransaction();

		if (fragment != null) {
			fragment.dismiss();
		}
	}

	private BinanceObserver mBinanaceObserver = new BinanceObserver() {
		@Override
		public void OnGetAccessToken(boolean isSuccess) {};

		@Override
		public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {};

		@Override
		public void OnGetConvertQuote(String quoteId, String quotePrice, String totalFee, String totalAmount) {};

		@Override
		public void OnGetCoinNetworks(String jsonNetworks) {};

		@Override
		public void OnGetDepositInfo(String depositAddress, String depositeTag, boolean isSuccess) {};

		@Override
		public void OnConfirmConvert(boolean isSuccess, String message) {};

		@Override
		public void OnGetConvertAssets(String jsonAssets) {
			// Log.e("NTP", "OnGetConvertAssets" + jsonAssets);
			try {
				binanceConvert = new BinanceConvert(jsonAssets);
				cryptoArray1 = (String[]) binanceConvert.getCurrencyKeys().toArray(new String[binanceConvert.getCurrencyKeys().size()]);
				ArrayAdapter<String> cryptoAdapter1 = new ArrayAdapter<String>(getActivity(),
				        R.layout.binance_spinner_text_layout, cryptoArray1);
				cryptoAdapter1.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
				cryptoSpinner1.setAdapter(cryptoAdapter1);

				selectedCrypto1 = cryptoArray1[0];
				setCryptoSpinner(selectedCrypto1);
				setTitle();
			} catch (JSONException e) {
				Log.e("NTP", e.getMessage());
			}
		};

		@Override
		public void OnRevokeToken(boolean isSuccess) {};
	};

	private void setCryptoSpinner(String key) {
		List<String> convertCryptoList = binanceConvert.getCurrencyValue(key);
		if (cryptoSpinner2 != null) {
			cryptoArray2 = (String[]) convertCryptoList.toArray(new String[convertCryptoList.size()]);
			ArrayAdapter<String> cryptoAdapter2 = new ArrayAdapter<String>(getActivity(),
			        R.layout.binance_spinner_text_layout, cryptoArray2);
			cryptoAdapter2.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			cryptoSpinner2.setAdapter(cryptoAdapter2);
			selectedCrypto2 = cryptoArray2[0];
		}
	}

	private void setTitle() {
		if (binanceConvertTitle != null) {
			BinanceAccountBalance binanceAccountBalance = BinanceWidgetManager.getInstance().getBinanceAccountBalance();
			binanceConvertTitle.setText(String.format(getResources().getString(R.string.available_balance_text), binanceAccountBalance.getCurrencyValue(selectedCrypto1) != null ? String.valueOf(binanceAccountBalance.getCurrencyValue(selectedCrypto1).first) : "0.000000" , selectedCrypto1));
		}
	}
}