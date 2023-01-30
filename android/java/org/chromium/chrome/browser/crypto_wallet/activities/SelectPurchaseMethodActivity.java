/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import androidx.appcompat.widget.Toolbar;

import org.chromium.brave_wallet.mojom.OnRampProvider;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.OnRampModel;
import org.chromium.chrome.browser.util.TabUtils;

public class SelectPurchaseMethodActivity extends BraveWalletBaseActivity {
    private static final String CHAIN_ID = "chainId";
    private static final String FROM = "from";
    private static final String RAMP_NETWORK_SYMBOL = "rampNetworkSymbol";
    private static final String AMOUNT = "amount";

    private OnRampModel mOnRampModel;

    private String mChainId;
    private String mFrom;
    private String mRampNetworkSymbol;
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

        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mOnRampModel = activity.getWalletModel().getCryptoModel().getOnRampModel();
        }

        Intent intent = getIntent();
        if (intent != null) {
            mChainId = intent.getStringExtra(CHAIN_ID);
            mFrom = intent.getStringExtra(FROM);
            mRampNetworkSymbol = intent.getStringExtra(RAMP_NETWORK_SYMBOL);
            mAmount = intent.getStringExtra(AMOUNT);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mRampNetworkLayout = findViewById(R.id.ramp_network_container);
        mSardineLayout = findViewById(R.id.sardine_container);
        mTransakLayout = findViewById(R.id.transak_container);

        mRampButton = findViewById(R.id.purchase_method_btn_ramp);
        mSardineButton = findViewById(R.id.purchase_method_btn_sardine);
        mTransakButton = findViewById(R.id.purchase_method_btn_transak);

        if (mOnRampModel != null) {
            mOnRampModel.getBuyUrl(
                    OnRampProvider.RAMP, mChainId, mFrom, mRampNetworkSymbol, mAmount, url -> {
                        if (url != null) {
                            enableOnRampService(mRampNetworkLayout, mRampButton, url);
                        }
                    });

            mOnRampModel.getBuyUrl(
                    OnRampProvider.SARDINE, mChainId, mFrom, mRampNetworkSymbol, mAmount, url -> {
                        if (url != null) {
                            enableOnRampService(mSardineLayout, mSardineButton, url);
                        }
                    });

            mOnRampModel.getBuyUrl(
                    OnRampProvider.TRANSAK, mChainId, mFrom, mRampNetworkSymbol, mAmount, url -> {
                        if (url != null) {
                            enableOnRampService(mTransakLayout, mTransakButton, url);
                        }
                    });
        }

        onInitialLayoutInflationComplete();
    }

    private void enableOnRampService(
            ViewGroup onRampLayout, Button onRampButton, String onRampUrl) {
        onRampButton.setOnClickListener(v -> {
            TabUtils.openUrlInNewTab(false, onRampUrl);
            TabUtils.bringChromeTabbedActivityToTheTop(this);
        });
        onRampLayout.setVisibility(View.VISIBLE);
    }

    public static Intent getIntent(
            Context context, String chainId, String from, String rampNetworkSymbol, String amount) {
        Intent intent = new Intent(context, SelectPurchaseMethodActivity.class);
        intent.putExtra(CHAIN_ID, chainId);
        intent.putExtra(FROM, from);
        intent.putExtra(RAMP_NETWORK_SYMBOL, rampNetworkSymbol);
        intent.putExtra(AMOUNT, amount);
        intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return intent;
    }
}
