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
public class BinanceBuyFragment extends Fragment implements  AdapterView.OnItemSelectedListener {
	private BinanceNativeWorker mBinanceNativeWorker;
	private CurrencyListAdapter mCurrencyListAdapter;

	private Button buyButton;

	private Spinner fiatSpinner;
	private Spinner cryptoSpinner;

	private String selectedFiat;
	private String selectedCrypto;

	private String[] fiatArray;
	private String[] cryptoArray;

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
		// Inflate the layout for this fragment
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

		RadioGroup buyRadioGroup = view.findViewById(R.id.buy_radio_group);
		EditText amountEditText = view.findViewById(R.id.amount_edittext);

		buyButton = view.findViewById(R.id.btn_buy);
		buyButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				String buyUrl = "";
				if (buyRadioGroup.getCheckedRadioButtonId() == R.id.com_radio) {
					buyUrl = String.format(BinanceWidgetManager.BINANCE_COM,  selectedFiat, selectedCrypto, amountEditText.getText());
				} else if (buyRadioGroup.getCheckedRadioButtonId() == R.id.us_radio) {
					buyUrl = String.format(BinanceWidgetManager.BINANCE_US, selectedCrypto, amountEditText.getText());
				}
				if (!TextUtils.isEmpty(buyUrl)) {
					TabUtils.openUrlInSameTab(buyUrl);
				}
				dismissBinanceBottomSheet();
			}
		});

		fiatSpinner = (Spinner)view.findViewById(R.id.fiat_spinner);
		fiatArray = (String[]) BinanceWidgetManager.fiatList.toArray(new String[BinanceWidgetManager.fiatList.size()]);
		fiatSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				selectedFiat = fiatArray[position];
			}

			@Override
			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here
			}

		});
		ArrayAdapter<String> fiatAdapter = new ArrayAdapter<String>(getActivity(),
		        R.layout.binance_spinner_text_layout, fiatArray);

		fiatAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		fiatSpinner.setAdapter(fiatAdapter);

		selectedFiat = fiatArray[0];

		cryptoSpinner = (Spinner)view.findViewById(R.id.crypto_spinner);
		cryptoSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
				selectedCrypto = cryptoArray[position];
				if (buyButton != null) {
					buyButton.setText(String.format(getResources().getString(R.string.buy_crypto_button_text), selectedCrypto));
				}
			}

			@Override
			public void onNothingSelected(AdapterView<?> parentView) {
				// your code here
			}

		});

		mBinanceNativeWorker.getCoinNetworks();
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
				List<String> cryptoList = new ArrayList<String>();
				for (CoinNetworkModel coinNetworkModel : binanceCoinNetworks.getCoinNetworksList()) {
					cryptoList.add(coinNetworkModel.getCoin());
				}
				if (cryptoSpinner != null) {
					cryptoArray = (String[]) cryptoList.toArray(new String[cryptoList.size()]);
					ArrayAdapter<String> cryptoAdapter = new ArrayAdapter<String>(getActivity(),
					        R.layout.binance_spinner_text_layout, cryptoArray);
					cryptoAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
					cryptoSpinner.setAdapter(cryptoAdapter);
					selectedCrypto = cryptoArray[0];
					buyButton.setText(String.format(getResources().getString(R.string.buy_crypto_button_text), selectedCrypto));
				}
			} catch (JSONException e) {
				Log.e("NTP", e.getMessage());
			}
		};

		@Override
		public void OnGetDepositInfo(String depositAddress, String depositeTag, boolean isSuccess) {};

		@Override
		public void OnConfirmConvert(boolean isSuccess, String message) {};

		@Override
		public void OnGetConvertAssets(String jsonAssets) {
			// Log.e("NTP", "OnGetConvertAssets"+jsonAssets);
		};

		@Override
		public void OnRevokeToken(boolean isSuccess) {};
	};

	@Override
	public void onItemSelected(AdapterView<?> arg0, View view, int position, long id) {
		if (R.id.fiat_spinner == view.getId()) {
			selectedFiat = fiatArray[position];
		} else if (R.id.crypto_spinner == view.getId()) {
			selectedCrypto = cryptoArray[position];
			if (buyButton != null) {
				buyButton.setText(String.format(getResources().getString(R.string.buy_crypto_button_text), selectedCrypto));
			}
		}
	}
	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
		// TODO Auto-generated method stub
	}
}