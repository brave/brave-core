/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class AssetDetailActivity
        extends AsyncInitializationActivity implements ConnectionErrorHandler {
    private SmoothLineChartEquallySpaced chartES;
    private AssetRatioController mAssetRatioController;

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_asset_detail);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView assetTitleText = findViewById(R.id.asset_title_text);
        assetTitleText.setText(this.getText(R.string.eth_name));

        RadioGroup radioGroup = findViewById(R.id.asset_duration_radio_group);
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            int timeframeType;
            if (checkedId == R.id.live_radiobutton) {
                timeframeType = AssetPriceTimeframe.LIVE;
            } else if (checkedId == R.id.day_1_radiobutton) {
                timeframeType = AssetPriceTimeframe.ONE_DAY;
            } else if (checkedId == R.id.week_1_radiobutton) {
                timeframeType = AssetPriceTimeframe.ONE_WEEK;
            } else if (checkedId == R.id.month_1_radiobutton) {
                timeframeType = AssetPriceTimeframe.ONE_MONTH;
            } else if (checkedId == R.id.month_3_radiobutton) {
                timeframeType = AssetPriceTimeframe.THREE_MONTHS;
            } else if (checkedId == R.id.year_1_radiobutton) {
                timeframeType = AssetPriceTimeframe.ONE_YEAR;
            } else {
                timeframeType = AssetPriceTimeframe.ALL;
            }
            getPriceHistory("eth", timeframeType);
        });

        chartES = findViewById(R.id.line_chart);
        chartES.setColors(new int[] {getResources().getColor(R.color.wallet_asset_graph_color)});
        chartES.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                v.getParent().requestDisallowInterceptTouchEvent(true);
                SmoothLineChartEquallySpaced chartES = (SmoothLineChartEquallySpaced) v;
                if (chartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    chartES.drawLine(event.getRawX(), findViewById(R.id.asset_price));
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    chartES.drawLine(-1, null);
                }

                return true;
            }
        });

        onInitialLayoutInflationComplete();
    }

    private void getPriceHistory(String asset, int timeframe) {
        if (mAssetRatioController != null) {
            mAssetRatioController.getPriceHistory(
                    asset, timeframe, (result, priceHistory) -> { chartES.setData(priceHistory); });
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
        InitAssetRatioController();
        getPriceHistory("eth", AssetPriceTimeframe.LIVE);
    }

    @Override
    public void onConnectionError(MojoException e) {
        mAssetRatioController = null;
        InitAssetRatioController();
    }

    private void InitAssetRatioController() {
        if (mAssetRatioController != null) {
            return;
        }

        mAssetRatioController =
                AssetRatioControllerFactory.getInstance().getAssetRatioController(this);
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}
