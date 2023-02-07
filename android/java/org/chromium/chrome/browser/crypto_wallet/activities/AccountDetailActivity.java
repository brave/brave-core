/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AccountDetailActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick, ApprovedTxObserver {
    private String mAddress;
    private String mName;
    private boolean mIsImported;
    private int mCoinType;
    private TextView mAccountText;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private WalletCoinAdapter mWalletTxCoinAdapter;
    private WalletModel mWalletModel;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_detail);

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
            mIsImported = getIntent().getBooleanExtra(Utils.ISIMPORTED, false);
            mCoinType = getIntent().getIntExtra(Utils.COIN_TYPE, CoinType.ETH);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle("");

        ImageView accountPicture = findViewById(R.id.account_picture);
        Utils.setBlockiesBitmapResource(mExecutor, mHandler, accountPicture, mAddress, true);

        mAccountText = findViewById(R.id.account_text);
        mAccountText.setText(mName);

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText(Utils.stripAccountAddress(mAddress));

        TextView btnDetails = findViewById(R.id.details_btn);
        btnDetails.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent accountDetailsWithQrActivityIntent =
                        new Intent(AccountDetailActivity.this, AccountDetailsWithQrActivity.class);
                accountDetailsWithQrActivityIntent.putExtra(Utils.ADDRESS, mAddress);
                accountDetailsWithQrActivityIntent.putExtra(Utils.NAME, mName);
                accountDetailsWithQrActivityIntent.putExtra(Utils.COIN_TYPE, mCoinType);
                startActivity(accountDetailsWithQrActivityIntent);
            }
        });
        TextView btnRename = findViewById(R.id.rename_btn);
        btnRename.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent addAccountActivityIntent =
                        new Intent(AccountDetailActivity.this, AddAccountActivity.class);
                addAccountActivityIntent.putExtra(Utils.ADDRESS, mAddress);
                addAccountActivityIntent.putExtra(Utils.NAME, mName);
                addAccountActivityIntent.putExtra(Utils.ISIMPORTED, mIsImported);
                addAccountActivityIntent.putExtra(Utils.ISUPDATEACCOUNT, true);
                CryptoAccountTypeInfo cryptoAccountTypeInfo;
                // TODO(sergz): Add other networks here
                if (mCoinType == CoinType.SOL) {
                    cryptoAccountTypeInfo = new CryptoAccountTypeInfo(
                            getString(R.string.brave_wallet_create_account_solana_description),
                            getString(R.string.wallet_sol_name), CoinType.SOL,
                            R.drawable.ic_sol_asset_icon);
                } else {
                    cryptoAccountTypeInfo = new CryptoAccountTypeInfo(
                            getString(R.string.brave_wallet_create_account_ethereum_description),
                            getString(R.string.wallet_eth_name), CoinType.ETH, R.drawable.eth);
                }
                addAccountActivityIntent.putExtra(
                        AddAccountActivity.ACCOUNT, cryptoAccountTypeInfo);
                startActivityForResult(addAccountActivityIntent, Utils.ACCOUNT_REQUEST_CODE);
            }
        });

        onInitialLayoutInflationComplete();
    }

    private void setUpAssetList(NetworkInfo selectedNetwork) {
        AccountInfo[] accountInfos = new AccountInfo[] {getThisAccountInfo()};
        if (mWalletModel == null) return;
        LiveDataUtil.observeOnce(
                mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks, allNetworks -> {
                    PortfolioHelper portfolioHelper =
                            new PortfolioHelper(this, allNetworks, accountInfos);
                    portfolioHelper.setSelectedNetwork(selectedNetwork);
                    portfolioHelper.calculateBalances(() -> {
                        RecyclerView rvAssets = findViewById(R.id.rv_assets);

                        BlockchainToken[] userAssets = portfolioHelper.getUserAssets();
                        HashMap<String, Double> perTokenCryptoSum =
                                portfolioHelper.getPerTokenCryptoSum();
                        HashMap<String, Double> perTokenFiatSum =
                                portfolioHelper.getPerTokenFiatSum();

                        String tokensPath =
                                BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

                        WalletCoinAdapter walletCoinAdapter = Utils.setupVisibleAssetList(
                                userAssets, perTokenCryptoSum, perTokenFiatSum, tokensPath);
                        walletCoinAdapter.setOnWalletListItemClick(AccountDetailActivity.this);
                        rvAssets.setAdapter(walletCoinAdapter);
                        rvAssets.setLayoutManager(new LinearLayoutManager(this));
                    });
                });
    }

    private void fetchAccountInfo(NetworkInfo selectedNetwork) {
        assert mKeyringService != null;
        mKeyringService.getKeyringInfo(AssetUtils.getKeyringForCoinType(mCoinType), keyringInfo -> {
            if (keyringInfo == null || mWalletModel == null) {
                return;
            }

            AccountInfo[] accounts = keyringInfo.accountInfos;
            LiveDataUtil.observeOnce(
                    mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks,
                    allNetworks -> {
                        Utils.getTxExtraInfo(this, allNetworks, selectedNetwork, accounts, null,
                                false,
                                (assetPrices, fullTokenList, nativeAssetsBalances,
                                        blockchainTokensBalances) -> {
                                    for (AccountInfo accountInfo : accounts) {
                                        if (accountInfo.address.equals(mAddress)
                                                && accountInfo.name.equals(mName)) {
                                            AccountInfo[] accountInfos = new AccountInfo[1];
                                            accountInfos[0] = accountInfo;
                                            WalletListItemModel thisAccountItemModel =
                                                    new WalletListItemModel(
                                                            Utils.getCoinIcon(mCoinType), mName,
                                                            mAddress, null, null, mIsImported);
                                            Utils.setUpTransactionList(this, accountInfos,
                                                    thisAccountItemModel, assetPrices,
                                                    fullTokenList, nativeAssetsBalances,
                                                    blockchainTokensBalances,
                                                    findViewById(R.id.rv_transactions), this,
                                                    mWalletTxCoinAdapter);
                                            break;
                                        }
                                    }
                                });
                    });
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                Intent returnIntent = new Intent();
                setResult(Activity.RESULT_OK, returnIntent);
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        initState();
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
        assert mJsonRpcService != null;
        mJsonRpcService.getNetwork(mCoinType, selectedNetwork -> {
            setUpAssetList(selectedNetwork);
            fetchAccountInfo(selectedNetwork);
        });
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        assert mJsonRpcService != null;
        mJsonRpcService.getChainId(mCoinType, chainId -> {
            Utils.openAssetDetailsActivity(AccountDetailActivity.this, chainId, asset);
        });
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        Utils.openTransaction(txInfo, mJsonRpcService, this, mName, mCoinType);
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String accountName, String txId) {}

    @Override
    public void onTxPending(String accountName, String txId) {}

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == Utils.ACCOUNT_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK && data != null) {
                mName = data.getStringExtra(Utils.NAME);
                if (mAccountText != null) {
                    mAccountText.setText(mName);
                }
            }
        }
    }

    @Override
    public void onBackPressed() {
        Intent returnIntent = new Intent();
        setResult(Activity.RESULT_OK, returnIntent);
        finish();
    }

    private AccountInfo getThisAccountInfo() {
        AccountInfo accountInfo = new AccountInfo();
        accountInfo.address = mAddress;
        accountInfo.name = mName;
        accountInfo.isImported = mIsImported;
        accountInfo.coin = mCoinType;
        accountInfo.keyringId = AssetUtils.getKeyringForCoinType(mCoinType);
        return accountInfo;
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        mWalletTxCoinAdapter.onTransactionUpdate(txInfo);
    }

    private void initState() {
        mWalletTxCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
    }
}
