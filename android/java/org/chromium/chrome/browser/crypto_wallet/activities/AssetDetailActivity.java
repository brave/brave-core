/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.ERCTokenRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.EthJsonRpcControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.EthTxControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountDetailActivity;
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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AssetDetailActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, OnWalletListItemClick {
    private SmoothLineChartEquallySpaced chartES;
    private AssetRatioController mAssetRatioController;
    private KeyringController mKeyringController;
    private EthTxController mEthTxController;
    private EthJsonRpcController mEthJsonRpcController;
    private int checkedTimeframeType;
    private String mAssetSymbol;
    private String mAssetName;
    private String mContractAddress;
    private String mAssetLogo;
    private int mAssetDecimals;
    private ExecutorService mExecutor;
    private Handler mHandler;

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mKeyringController.close();
        mAssetRatioController.close();
        mEthTxController.close();
        mEthJsonRpcController.close();
    }

    @Override
    public void onUserInteraction() {
        if (mKeyringController == null) {
            return;
        }
        mKeyringController.notifyUserInteraction();
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_asset_detail);

        if (getIntent() != null) {
            mAssetSymbol = getIntent().getStringExtra(Utils.ASSET_SYMBOL);
            mAssetName = getIntent().getStringExtra(Utils.ASSET_NAME);
            mContractAddress = getIntent().getStringExtra(Utils.ASSET_CONTRACT_ADDRESS);
            mAssetLogo = getIntent().getStringExtra(Utils.ASSET_LOGO);
            mAssetDecimals = getIntent().getIntExtra(Utils.ASSET_DECIMALS, 18);
        }
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView assetTitleText = findViewById(R.id.asset_title_text);
        assetTitleText.setText(mAssetName);
        String tokensPath = ERCTokenRegistryFactory.getInstance().getTokensIconsLocation();
        String iconPath = mAssetLogo.isEmpty() ? null : ("file://" + tokensPath + "/" + mAssetLogo);
        Utils.setBitmapResource(mExecutor, mHandler, this, iconPath, R.drawable.ic_eth_24, null,
                assetTitleText, false);

        TextView assetPriceText = findViewById(R.id.asset_price_text);
        assetPriceText.setText(String.format(
                getResources().getString(R.string.asset_price), mAssetName, mAssetSymbol));

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
        checkedTimeframeType = radioGroup.getCheckedRadioButtonId();
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            int leftDot = R.drawable.ic_live_dot;
            ((RadioButton) findViewById(checkedTimeframeType))
                    .setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            RadioButton button = findViewById(checkedId);
            button.setCompoundDrawablesWithIntrinsicBounds(leftDot, 0, 0, 0);
            int timeframeType = Utils.getTimeframeFromRadioButtonId(checkedId);
            getPriceHistory(mAssetSymbol, timeframeType);
            checkedTimeframeType = checkedId;
        });

        chartES = findViewById(R.id.line_chart);
        chartES.setColors(new int[] {getResources().getColor(R.color.wallet_asset_graph_color)});
        chartES.drawLine(0.f, findViewById(R.id.asset_price));
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

    private void setUpAccountList() {
        RecyclerView rvAccounts = findViewById(R.id.rv_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        KeyringController keyringController = getKeyringController();
        if (keyringController != null) {
            keyringController.getDefaultKeyringInfo(keyringInfo -> {
                if (keyringInfo != null) {
                    AccountInfo[] accountInfos = keyringInfo.accountInfos;
                    Utils.setUpTransactionList(accountInfos, mAssetRatioController,
                            mEthTxController, null, mAssetSymbol, mContractAddress, mAssetDecimals,
                            findViewById(R.id.rv_transactions), this, this, null);
                    List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
                    for (AccountInfo accountInfo : accountInfos) {
                        walletListItemModelList.add(
                                new WalletListItemModel(R.drawable.ic_eth, accountInfo.name,
                                        accountInfo.address, null, null, accountInfo.isImported));
                    }
                    if (walletCoinAdapter != null) {
                        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
                        walletCoinAdapter.setOnWalletListItemClick(AssetDetailActivity.this);
                        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
                        rvAccounts.setAdapter(walletCoinAdapter);
                        rvAccounts.setLayoutManager(
                                new LinearLayoutManager(AssetDetailActivity.this));
                    }
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
        InitAssetRatioController();
        InitKeyringController();
        InitEthTxController();
        InitEthJsonRpcController();
        getPriceHistory(mAssetSymbol, AssetPriceTimeframe.ONE_DAY);
        setUpAccountList();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringController.close();
        mAssetRatioController.close();
        mEthTxController.close();
        mEthJsonRpcController.close();

        mAssetRatioController = null;
        InitAssetRatioController();

        mKeyringController = null;
        InitKeyringController();

        mEthTxController = null;
        InitEthTxController();

        mEthJsonRpcController = null;
        InitEthJsonRpcController();
    }

    private void InitAssetRatioController() {
        if (mAssetRatioController != null) {
            return;
        }

        mAssetRatioController =
                AssetRatioControllerFactory.getInstance().getAssetRatioController(this);
    }

    private void InitEthTxController() {
        if (mEthTxController != null) {
            return;
        }

        mEthTxController = EthTxControllerFactory.getInstance().getEthTxController(this);
    }

    private void InitEthJsonRpcController() {
        if (mEthJsonRpcController != null) {
            return;
        }

        mEthJsonRpcController =
                EthJsonRpcControllerFactory.getInstance().getEthJsonRpcController(this);
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void onAccountClick(WalletListItemModel walletListItemModel) {
        Intent accountDetailActivityIntent =
                new Intent(AssetDetailActivity.this, AccountDetailActivity.class);
        accountDetailActivityIntent.putExtra(Utils.NAME, walletListItemModel.getTitle());
        accountDetailActivityIntent.putExtra(Utils.ADDRESS, walletListItemModel.getSubTitle());
        accountDetailActivityIntent.putExtra(
                Utils.ISIMPORTED, walletListItemModel.getIsImportedAccount());
        startActivityForResult(accountDetailActivityIntent, Utils.ACCOUNT_REQUEST_CODE);
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        Utils.openTransaction(txInfo, mEthJsonRpcController, this);
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }

    public KeyringController getKeyringController() {
        return mKeyringController;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == Utils.ACCOUNT_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                setUpAccountList();
            }
        }
    }
}
