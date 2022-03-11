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
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.TxServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.SingleTokenBalanceHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AssetDetailActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick, ApprovedTxObserver {
    private SmoothLineChartEquallySpaced chartES;
    private int checkedTimeframeType;
    private String mAssetSymbol;
    private String mAssetName;
    private String mContractAddress;
    private String mAssetLogo;
    private int mAssetDecimals;
    private String mChainId;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private AccountInfo[] accountInfos;
    private WalletCoinAdapter mWalletTxCoinAdapter;
    private boolean mHasNewTx;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_asset_detail);

        if (getIntent() != null) {
            mChainId = getIntent().getStringExtra(Utils.CHAIN_ID);
            mAssetSymbol = getIntent().getStringExtra(Utils.ASSET_SYMBOL);
            mAssetName = getIntent().getStringExtra(Utils.ASSET_NAME);
            mContractAddress = getIntent().getStringExtra(Utils.ASSET_CONTRACT_ADDRESS);
            mAssetLogo = getIntent().getStringExtra(Utils.ASSET_LOGO);
            mAssetDecimals = getIntent().getIntExtra(Utils.ASSET_DECIMALS, 18);
            if (mAssetSymbol.equals("ETH")) {
                mAssetLogo = "eth.png";
            }
        }
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView assetTitleText = findViewById(R.id.asset_title_text);
        assetTitleText.setText(mAssetName);
        if (!mAssetLogo.isEmpty()) {
            String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
            String iconPath =
                    mAssetLogo.isEmpty() ? null : ("file://" + tokensPath + "/" + mAssetLogo);
            Utils.setBitmapResource(mExecutor, mHandler, this, iconPath, R.drawable.ic_eth_24, null,
                    assetTitleText, false);
        } else {
            Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, mContractAddress,
                    mAssetSymbol, getResources().getDisplayMetrics().density, assetTitleText, this,
                    false, (float) 0.5);
        }

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
                Utils.openBuySendSwapActivity(AssetDetailActivity.this,
                        BuySendSwapActivity.ActivityType.SWAP, mAssetSymbol);
            }
        });
        if (!Utils.isCustomNetwork(mChainId)) {
            btnBuy.setVisibility(View.VISIBLE);
            btnSwap.setVisibility(View.VISIBLE);
        } else {
            btnBuy.setVisibility(View.GONE);
            btnSwap.setVisibility(View.GONE);
        }

        RadioGroup radioGroup = findViewById(R.id.asset_duration_radio_group);
        checkedTimeframeType = radioGroup.getCheckedRadioButtonId();
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            ((RadioButton) findViewById(checkedTimeframeType))
                    .setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            RadioButton button = findViewById(checkedId);
            int timeframeType = Utils.getTimeframeFromRadioButtonId(checkedId);
            getPriceHistory(mAssetSymbol, "usd", timeframeType);
            checkedTimeframeType = checkedId;
        });

        final TextView assetPrice = findViewById(R.id.asset_price);
        chartES = findViewById(R.id.line_chart);
        chartES.setColors(new int[] {getResources().getColor(R.color.wallet_asset_graph_color)});
        chartES.drawLine(0, assetPrice);
        chartES.setNoDrawText(true);
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
                    chartES.drawLine(event.getRawX(), assetPrice);
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    chartES.drawLine(-1, null);
                }

                return true;
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mHasNewTx) {
            setUpAccountList();
            mHasNewTx = false;
        }
    }

    private void getPriceHistory(String asset, String vsAsset, int timeframe) {
        if (mAssetRatioService != null) {
            mAssetRatioService.getPriceHistory(asset, vsAsset, timeframe,
                    (result, priceHistory) -> { chartES.setData(priceHistory); });
        }
    }

    private void getPrice(String asset, String vsAsset, int timeframe) {
        assert mAssetRatioService != null;
        String[] fromAssets = new String[] {asset.toLowerCase(Locale.getDefault())};
        String[] toAssets = new String[] {vsAsset.toLowerCase(Locale.getDefault())};
        mAssetRatioService.getPrice(fromAssets, toAssets, timeframe, (success, price) -> {
            if (!success && price.length == 0) {
                return;
            }
            String btcPriceText = price[0].price + " BTC";
            TextView btcPrice = findViewById(R.id.asset_price_btc_text);
            btcPrice.setText(btcPriceText);
        });
    }

    private void setUpAccountList() {
        RecyclerView rvAccounts = findViewById(R.id.rv_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        mWalletTxCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        KeyringService keyringService = getKeyringService();
        if (keyringService != null) {
            keyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
                if (keyringInfo != null) {
                    accountInfos = keyringInfo.accountInfos;
                    Utils.setUpTransactionList(accountInfos, mAssetRatioService, mTxService, null,
                            null, mAssetSymbol, mContractAddress, mAssetDecimals,
                            findViewById(R.id.rv_transactions), this, this, mChainId,
                            mJsonRpcService, mWalletTxCoinAdapter);

                    SingleTokenBalanceHelper singleTokenBalanceHelper =
                            new SingleTokenBalanceHelper(
                                    getAssetRatioService(), getJsonRpcService());
                    singleTokenBalanceHelper.getPerAccountBalances(mChainId, mContractAddress,
                            mAssetSymbol, mAssetDecimals, accountInfos, () -> {
                                List<WalletListItemModel> walletListItemModelList =
                                        new ArrayList<>();
                                for (AccountInfo accountInfo : accountInfos) {
                                    Double thisAccountFiatBalance = Utils.getOrDefault(
                                            singleTokenBalanceHelper.getPerAccountFiatBalance(),
                                            accountInfo.address, 0.0d);
                                    final String fiatBalanceString = String.format(
                                            Locale.getDefault(), "$%,.2f", thisAccountFiatBalance);

                                    Double thisAccountCryptoBalance = Utils.getOrDefault(
                                            singleTokenBalanceHelper.getPerAccountCryptoBalance(),
                                            accountInfo.address, 0.0d);
                                    final String cryptoBalanceString =
                                            String.format(Locale.getDefault(), "%.4f %s",
                                                    thisAccountCryptoBalance, mAssetSymbol);

                                    walletListItemModelList.add(new WalletListItemModel(
                                            R.drawable.ic_eth, accountInfo.name,
                                            accountInfo.address, fiatBalanceString,
                                            cryptoBalanceString, accountInfo.isImported));

                                    if (walletCoinAdapter != null) {
                                        walletCoinAdapter.setWalletListItemModelList(
                                                walletListItemModelList);
                                        walletCoinAdapter.setOnWalletListItemClick(
                                                AssetDetailActivity.this);
                                        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
                                        rvAccounts.setAdapter(walletCoinAdapter);
                                        rvAccounts.setLayoutManager(
                                                new LinearLayoutManager(AssetDetailActivity.this));
                                    }
                                }
                            });
                }
            });
        }
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        getPriceHistory(mAssetSymbol, "usd", AssetPriceTimeframe.ONE_DAY);
        getPrice(mAssetSymbol, "btc", AssetPriceTimeframe.LIVE);
        setUpAccountList();
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
        Utils.openTransaction(txInfo, mJsonRpcService, this, accountInfos);
    }

    @Override
    public void OnTxApprovedRejected(boolean approved, String accountName, String txId) {}

    @Override
    public void OnTxPending(String accountName, String txId) {}

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        mWalletTxCoinAdapter.onTransactionUpdate(txInfo);
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        mHasNewTx = true;
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
