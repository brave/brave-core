/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import androidx.annotation.StringRes;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.OnRampProvider;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.BuyModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

public class SelectPurchaseMethodActivity extends BraveWalletBaseActivity {
    private static final String CHAIN_ID = "chainId";
    private static final String FROM = "from";
    private static final String ASSET_SYMBOL = "assetSymbol";
    private static final String CONTRACT_ADDRESS = "contractAddress";
    private static final String AMOUNT = "amount";
    private static final String TAG = "SelectPurchase";

    private BuyModel mBuyModel;
    private WalletModel mWalletModel;

    private String mChainId;
    private String mFrom;
    private String mAssetSymbol;
    private String mContractAddress;
    private String mAmount;

    private ViewGroup mRampNetworkLayout;
    private ViewGroup mSardineLayout;
    private ViewGroup mTransakLayout;
    private Button mRampButton;
    private Button mSardineButton;
    private Button mTransakButton;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_select_purchase_method);

        Intent intent = getIntent();
        if (intent != null) {
            mChainId = intent.getStringExtra(CHAIN_ID);
            mFrom = intent.getStringExtra(FROM);
            mAssetSymbol = intent.getStringExtra(ASSET_SYMBOL);
            mContractAddress = intent.getStringExtra(CONTRACT_ADDRESS);
            mAmount = intent.getStringExtra(AMOUNT);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mRampNetworkLayout = findViewById(R.id.ramp_network_container);
        mSardineLayout = findViewById(R.id.sardine_container);
        mTransakLayout = findViewById(R.id.transak_container);

        mRampButton = findViewById(R.id.purchase_method_btn_ramp);
        mRampButton.setText(getRampProviderTextButton(R.string.brave_wallet_ramp_network_short));
        mSardineButton = findViewById(R.id.purchase_method_btn_sardine);
        mSardineButton.setText(getRampProviderTextButton(R.string.brave_wallet_sardine_title));
        mTransakButton = findViewById(R.id.purchase_method_btn_transak);
        mTransakButton.setText(getRampProviderTextButton(R.string.brave_wallet_transak_title));

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            if (mWalletModel != null && mWalletModel.getCryptoModel() != null) {
                mBuyModel = mWalletModel.getCryptoModel().getBuyModel();
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "finishNativeInitialization " + e);
        }
        if (mWalletModel != null && mBuyModel != null) {
            LiveDataUtil.observeOnce(
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork,
                    selectedNetwork -> {
                        mBuyModel.isBuySupported(selectedNetwork, mAssetSymbol, mContractAddress,
                                mChainId, new int[] {OnRampProvider.RAMP}, isBuyEnabled -> {
                                    setupOnRampService(isBuyEnabled, OnRampProvider.RAMP,
                                            mRampNetworkLayout, mRampButton);
                                });

                        mBuyModel.isBuySupported(selectedNetwork, mAssetSymbol, mContractAddress,
                                mChainId, new int[] {OnRampProvider.SARDINE}, isBuyEnabled -> {
                                    setupOnRampService(isBuyEnabled, OnRampProvider.SARDINE,
                                            mSardineLayout, mSardineButton);
                                });

                        mBuyModel.isBuySupported(selectedNetwork, mAssetSymbol, mContractAddress,
                                mChainId, new int[] {OnRampProvider.TRANSAK}, isBuyEnabled -> {
                                    setupOnRampService(isBuyEnabled, OnRampProvider.TRANSAK,
                                            mTransakLayout, mTransakButton);
                                });
                    });
        }
    }

    private String getRampProviderTextButton(@StringRes int providerNameResource) {
        return String.format(getString(R.string.brave_wallet_buy_with_ramp_provider),
                getString(providerNameResource));
    }

    private void setupOnRampService(
            boolean isBuyEnabled, int onRampProvider, ViewGroup onRampLayout, Button onRampButton) {
        if (isBuyEnabled && mBuyModel.isAvailable(onRampProvider, getResources())) {
            onRampLayout.setVisibility(View.VISIBLE);
            mBuyModel.getBuyUrl(onRampProvider, mChainId, mFrom, mAssetSymbol, mAmount,
                    mContractAddress,
                    url -> { enableOnRampService(onRampLayout, onRampButton, url); });
        }
    }

    private void enableOnRampService(
            ViewGroup onRampLayout, Button onRampButton, String onRampUrl) {
        if (onRampUrl == null) {
            onRampLayout.setVisibility(View.GONE);
        } else {
            onRampButton.setOnClickListener(v -> {
                TabUtils.openUrlInNewTab(false, onRampUrl);
                TabUtils.bringChromeTabbedActivityToTheTop(this);
            });
        }
    }

    public static Intent getIntent(Context context, String chainId, String from,
            String rampNetworkSymbol, String contractAddress, String amount) {
        Intent intent = new Intent(context, SelectPurchaseMethodActivity.class);
        intent.putExtra(CHAIN_ID, chainId);
        intent.putExtra(FROM, from);
        intent.putExtra(ASSET_SYMBOL, rampNetworkSymbol);
        intent.putExtra(CONTRACT_ADDRESS, contractAddress);
        intent.putExtra(AMOUNT, amount);
        intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return intent;
    }
}
