/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.os.Bundle;
import android.view.MenuItem;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletObserver;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

public class BuySendSwapActivity
        extends AsyncInitializationActivity implements BraveWalletObserver {
    @Override
    protected void onDestroy() {
        BraveWalletNativeWorker.getInstance().removeObserver(this);
        super.onDestroy();
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_buy_send_swap);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView accountNameText = findViewById(R.id.account_name_text);
        accountNameText.setText("Account 1");

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText("0xFCdF***DDee");

        TextView fromValueText = findViewById(R.id.from_value_text);
        fromValueText.setText("0xFCdF***DDee");

        TextView fromBalanceText = findViewById(R.id.from_balance_text);
        fromBalanceText.setText("Balance: 1.2832");

        TextView fromAssetText = findViewById(R.id.from_asset_text);
        fromAssetText.setText("ETH");

        TextView toValueText = findViewById(R.id.to_value_text);
        toValueText.setText("561.121");

        TextView toBalanceText = findViewById(R.id.to_balance_text);
        toBalanceText.setText("Balance: 0");

        TextView toAssetText = findViewById(R.id.to_asset_text);
        toAssetText.setText("ETH");

        TextView marketPriceValueText = findViewById(R.id.market_price_value_text);
        marketPriceValueText.setText("0.0005841");

        TextView slippingToleranceValueText = findViewById(R.id.slipping_tolerance_value_text);
        slippingToleranceValueText.setText("2%");

        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        BraveWalletNativeWorker.getInstance().addObserver(this);
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}
