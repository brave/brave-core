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
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;

public class AssetDetailActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, OnWalletListItemClick {
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

        Button btnBuy = findViewById(R.id.btn_buy);
        btnBuy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Utils.openBuySendSwapActivity(
                        AssetDetailActivity.this, BuySendSwapActivity.ActivityType.BUY);
            }
        });
        Button btnSend = findViewById(R.id.btn_send);
        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Utils.openBuySendSwapActivity(
                        AssetDetailActivity.this, BuySendSwapActivity.ActivityType.SEND);
            }
        });
        Button btnSwap = findViewById(R.id.btn_swap);
        btnSwap.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Utils.openBuySendSwapActivity(
                        AssetDetailActivity.this, BuySendSwapActivity.ActivityType.SWAP);
            }
        });

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

        setUpAccountList();
        setUpTransactionList();

        onInitialLayoutInflationComplete();
    }

    private void getPriceHistory(String asset, int timeframe) {
        if (mAssetRatioController != null) {
            mAssetRatioController.getPriceHistory(
                    asset, timeframe, (result, priceHistory) -> { chartES.setData(priceHistory); });
        }
    }

    private void setUpAccountList() {
        RecyclerView rvAccounts = findViewById(R.id.rv_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Account 1", "0xFCdF***DDee", "$616.47", "0.31178 ETH"));
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Ledger Nano", "0xA1da***7af1", "$256.01", "0.0121 ETH"));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AssetDetailActivity.this);
        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
        rvAccounts.setAdapter(walletCoinAdapter);
        rvAccounts.setLayoutManager(new LinearLayoutManager(this));
    }

    private void setUpTransactionList() {
        RecyclerView rvTransactions = findViewById(R.id.rv_transactions);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Ledger Nano", "0xA1da***7af1", "$37.92", "0.0009431 ETH"));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AssetDetailActivity.this);
        walletCoinAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        rvTransactions.setAdapter(walletCoinAdapter);
        rvTransactions.setLayoutManager(new LinearLayoutManager(this));
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

    @Override
    public void onAccountClick() {
        Utils.openAccountDetailActivity(AssetDetailActivity.this);
    }

    @Override
    public void onTransactionClick() {}
}
