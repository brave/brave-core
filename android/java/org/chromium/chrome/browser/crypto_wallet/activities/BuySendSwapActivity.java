/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

import java.util.HashMap;
import java.util.Map;

public class BuySendSwapActivity extends AsyncInitializationActivity {
    public enum ActivityType {
        BUY(0),
        SEND(1),
        SWAP(2);

        private int value;
        private static Map map = new HashMap<>();

        private ActivityType(int value) {
            this.value = value;
        }

        static {
            for (ActivityType activityType : ActivityType.values()) {
                map.put(activityType.value, activityType);
            }
        }

        public static ActivityType valueOf(int activityType) {
            return (ActivityType) map.get(activityType);
        }

        public int getValue() {
            return value;
        }
    }

    private ActivityType mActivityType;

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_buy_send_swap);

        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra("activityType", ActivityType.BUY.getValue()));

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView accountNameText = findViewById(R.id.account_name_text);
        accountNameText.setText("Account 1");

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText("0xFCdF***DDee");

        TextView fromValueText = findViewById(R.id.from_value_text);
        fromValueText.setText("");
        fromValueText.setHint("0");

        TextView fromBalanceText = findViewById(R.id.from_balance_text);
        fromBalanceText.setText("Balance: 1.2832");

        TextView fromAssetText = findViewById(R.id.from_asset_text);
        fromAssetText.setText("ETH");

        EditText toValueText = findViewById(R.id.to_value_text);
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

        adjustControls();
    }

    private void adjustControls() {
        EditText toValueText = findViewById(R.id.to_value_text);
        TextView marketPriceValueText = findViewById(R.id.market_price_value_text);
        TextView slippingToleranceValueText = findViewById(R.id.slipping_tolerance_value_text);
        RadioGroup radioBuySendSwap = findViewById(R.id.buy_send_swap_type_radio_group);
        LinearLayout marketPriceSection = findViewById(R.id.market_price_section);
        LinearLayout toleranceSection = findViewById(R.id.tolerance_section);
        Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        TextView currencySign = findViewById(R.id.currency_sign);
        TextView toEstimateText = findViewById(R.id.to_estimate_text);
        if (mActivityType == ActivityType.BUY) {
            TextView fromBuyText = findViewById(R.id.from_buy_text);
            fromBuyText.setText(getText(R.string.buy_wallet));
            ImageView arrowDown = findViewById(R.id.arrow_down);
            arrowDown.setVisibility(View.GONE);
            LinearLayout toSection = findViewById(R.id.to_section);
            toSection.setVisibility(View.GONE);
            radioBuySendSwap.setVisibility(View.GONE);
            marketPriceSection.setVisibility(View.GONE);
            toleranceSection.setVisibility(View.GONE);
            btnBuySendSwap.setText(getText(R.string.buy_wallet));
            RadioGroup radioPerPercent = findViewById(R.id.per_percent_radiogroup);
            radioPerPercent.setVisibility(View.GONE);
        } else if (mActivityType == ActivityType.SEND) {
            currencySign.setVisibility(View.GONE);
            toEstimateText.setText(getText(R.string.to_address));
            toValueText.setText("");
            toValueText.setHint(getText(R.string.to_address_edit));
            radioBuySendSwap.setVisibility(View.GONE);
            marketPriceSection.setVisibility(View.GONE);
            toleranceSection.setVisibility(View.GONE);
            btnBuySendSwap.setText(getText(R.string.send));
            LinearLayout toBalanceSection = findViewById(R.id.to_balance_section);
            toBalanceSection.setVisibility(View.GONE);
        } else if (mActivityType == ActivityType.SWAP) {
            currencySign.setVisibility(View.GONE);
            toValueText.setText("");
            toValueText.setHint("0");
            Button btMarket = findViewById(R.id.market_radio);
            Button btLimit = findViewById(R.id.limit_radio);
            TextView marketLimitPriceText = findViewById(R.id.market_limit_price_text);
            EditText limitPriceValue = findViewById(R.id.limit_price_value);
            TextView slippingExpiresValueText = findViewById(R.id.slipping_expires_value_text);
            ImageView refreshPrice = findViewById(R.id.refresh_price);
            btMarket.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    toEstimateText.setText(getText(R.string.to_estimate));
                    marketLimitPriceText.setText(getText(R.string.market_price_in));
                    marketPriceValueText.setVisibility(View.VISIBLE);
                    limitPriceValue.setVisibility(View.GONE);
                    marketPriceValueText.setVisibility(View.VISIBLE);
                    slippingExpiresValueText.setText(getText(R.string.slipping_tolerance));
                    slippingToleranceValueText.setText("2%");
                    refreshPrice.setVisibility(View.VISIBLE);
                }
            });
            btLimit.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    toEstimateText.setText(getText(R.string.to_address));
                    marketLimitPriceText.setText(getText(R.string.price_in));
                    marketPriceValueText.setVisibility(View.GONE);
                    limitPriceValue.setVisibility(View.VISIBLE);
                    marketPriceValueText.setVisibility(View.GONE);
                    slippingExpiresValueText.setText(getText(R.string.expires_in));
                    slippingToleranceValueText.setText("1 days");
                    refreshPrice.setVisibility(View.GONE);
                }
            });
        }
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
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}
