/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.util.Collections;

public class AddAssetActivity extends BraveWalletBaseActivity {
    private static final String TAG = "AddAssetActivity";

    private WalletModel mWalletModel;
    private NetworkModel mNetworkModel;

    private boolean mNftsOnly;

    private Toolbar mToolbar;
    private Spinner mNetworkSpinner;
    private View mAdvancedSection;
    private TextView mTvDecimalTitle;
    private EditText mTokenDecimalsEdit;
    private TextView mTvAddressTitle;
    private NetworkSpinnerAdapter mNetworkAdapter;

    public static Intent getIntent(@NonNull Context context) {
        Intent intent = new Intent(context, AddAssetActivity.class);
        // intent.putExtra(Utils.COIN_TYPE, coinForNewAccount);
        return intent;
    }

    @Override
    protected void onPreCreate() {
        Intent intent = getIntent();
        //        if (intent != null) {
        //            mCoinForNewAccount = intent.getIntExtra(Utils.COIN_TYPE, -1);
        //            mEditedAccountInfo = WalletUtils.getAccountInfoFromIntent(intent);
        //        }
        //        mSelectedFilecoinNetwork = BraveWalletConstants.FILECOIN_MAINNET;
    }

    @Override
    protected void triggerLayoutInflation() {
        Log.d(TAG, "triggerLayoutInflation");
        setContentView(R.layout.activity_add_asset);

        mToolbar = findViewById(R.id.toolbar);
        setSupportActionBar(mToolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mNetworkSpinner = findViewById(R.id.network_spinner);
        mNetworkAdapter = new NetworkSpinnerAdapter(this, Collections.emptyList());
        mNetworkAdapter.mNetworkTitleSize = 14;
        mNetworkSpinner.setAdapter(mNetworkAdapter);
        mAdvancedSection = findViewById(R.id.advanced_section);
        mTvDecimalTitle = findViewById(R.id.token_decimals_title);
        mTokenDecimalsEdit = findViewById(R.id.token_decimals);
        mTvAddressTitle = findViewById(R.id.token_contract_address_title);

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mNetworkModel = mWalletModel.getNetworkModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error onStartWithNative.", e);
        }

        LiveDataUtil.observeOnce(mNetworkModel.mCryptoNetworks,
                networkInfoList -> { mNetworkAdapter.setNetworks(networkInfoList); });

        mNetworkSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                NetworkInfo network = mNetworkAdapter.getNetwork(position);
                // filterAddCustomAssetTextWatcher.setNetworkInfo(network);
                if (network.coin == CoinType.ETH) {
                    mAdvancedSection.setVisibility(mNftsOnly ? View.VISIBLE : View.GONE);
                } else {
                    if (network.coin == CoinType.SOL) {
                        if (mNftsOnly) {
                            AndroidUtils.gone(mTvDecimalTitle, mTokenDecimalsEdit);
                        } else {
                            AndroidUtils.show(mTvDecimalTitle, mTokenDecimalsEdit);
                        }
                        mTvAddressTitle.setText(getString(mNftsOnly
                                        ? R.string.wallet_add_custom_asset_token_address
                                        : R.string.wallet_add_custom_asset_token_contract_address));
                    }
                }
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) { /* No-op. */
            }
        });
    }
}
