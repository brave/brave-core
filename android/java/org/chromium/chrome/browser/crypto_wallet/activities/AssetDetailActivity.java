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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletObserver;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

import java.util.ArrayList;
import java.util.List;

public class AssetDetailActivity
        extends AsyncInitializationActivity implements BraveWalletObserver {
    private SmoothLineChartEquallySpaced chartES;

    @Override
    protected void onDestroy() {
        BraveWalletNativeWorker.getInstance().removeObserver(this);
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
                timeframeType = 0;
            } else if (checkedId == R.id.day_1_radiobutton) {
                timeframeType = 1;
            } else if (checkedId == R.id.week_1_radiobutton) {
                timeframeType = 2;
            } else if (checkedId == R.id.month_1_radiobutton) {
                timeframeType = 3;
            } else if (checkedId == R.id.month_3_radiobutton) {
                timeframeType = 4;
            } else if (checkedId == R.id.year_1_radiobutton) {
                timeframeType = 5;
            } else {
                timeframeType = 6;
            }
            BraveWalletNativeWorker.getInstance().getAssetPriceHistory("eth", timeframeType);
        });

        chartES = findViewById(R.id.line_chart);
        chartES.setColors(new int[] {getResources().getColor(R.color.wallet_asset_graph_color)});
        chartES.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                SmoothLineChartEquallySpaced chartES = (SmoothLineChartEquallySpaced) v;
                if (chartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    chartES.drawLine(event.getRawX());
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    chartES.drawLine(-1);
                }

                return true;
            }
        });

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
        BraveWalletNativeWorker.getInstance().getAssetPriceHistory("eth", 0);
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    private void updateAssetGraph(float[] prices) {
        chartES.setData(prices);
    }

    @Override
    public void OnGetPriceHistory(String priceHistory, boolean isSuccess) {
        List<Float> pricesList = parsePriceHistory(priceHistory);
        final float[] pricesArray = new float[pricesList.size()];
        int index = 0;
        for (final Float value : pricesList) {
            pricesArray[index++] = value;
        }
        updateAssetGraph(pricesArray);
    }

    private List<Float> parsePriceHistory(String priceHistory) {
        List<Float> priceList = new ArrayList<>();
        // Add root element to make it real JSON, otherwise getJSONArray cannot parse it
        priceHistory = "{\"prices\":" + priceHistory + "}";
        try {
            JSONObject result = new JSONObject(priceHistory);
            JSONArray prices = result.getJSONArray("prices");
            for (int i = 0; i < prices.length(); i++) {
                JSONObject price = prices.getJSONObject(i);
                priceList.add((float) price.getDouble("price"));
            }
        } catch (JSONException e) {
            Log.e("NTP", "parsePriceHistory JSONException error " + e);
        } catch (IllegalStateException e) {
            Log.e("NTP", "parsePriceHistory IllegalStateException error " + e);
        }
        return priceList;
    }
}
