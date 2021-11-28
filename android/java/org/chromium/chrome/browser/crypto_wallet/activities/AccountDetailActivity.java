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
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.ERCTokenRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.EthJsonRpcControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.EthTxControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AccountDetailActivity extends AsyncInitializationActivity
        implements OnWalletListItemClick, ConnectionErrorHandler {
    private String mAddress;
    private String mName;
    private boolean mIsImported;
    private TextView mAccountText;
    private ExecutorService mExecutor;
    private Handler mHandler;

    private ErcTokenRegistry mErcTokenRegistry;
    private EthTxController mEthTxController;
    private EthJsonRpcController mEthJsonRpcController;
    private AssetRatioController mAssetRatioController;
    private BraveWalletService mBraveWalletService;
    private KeyringController mKeyringController;

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mAssetRatioController.close();
        mEthJsonRpcController.close();
        mBraveWalletService.close();
        mKeyringController.close();
        mEthTxController.close();
        mErcTokenRegistry.close();
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_detail);

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
            mIsImported = getIntent().getBooleanExtra(Utils.ISIMPORTED, false);
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
                startActivityForResult(addAccountActivityIntent, Utils.ACCOUNT_REQUEST_CODE);
            }
        });

        InitEthJsonRpcController();
        InitAssetRatioController();
        InitBraveWalletService();
        InitKeyringController();
        InitEthTxController();
        InitErcTokenRegistry();

        assert mEthJsonRpcController != null;
        mEthJsonRpcController.getChainId(chainId -> {
            setUpAssetList(chainId);
            fetchAccountInfo(chainId);
        });

        onInitialLayoutInflationComplete();
    }

    private void setUpAssetList(String chainId) {
        AccountInfo[] accountInfos = new AccountInfo[] {getThisAccountInfo()};
        PortfolioHelper portfolioHelper = new PortfolioHelper(getBraveWalletService(),
                getAssetRatioController(), mEthJsonRpcController, accountInfos);
        portfolioHelper.setChainId(chainId);
        portfolioHelper.calculateBalances(() -> {
            RecyclerView rvAssets = findViewById(R.id.rv_assets);
            WalletCoinAdapter walletCoinAdapter =
                    new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
            List<WalletListItemModel> walletListItemModelList = new ArrayList<>();

            String tokensPath = ERCTokenRegistryFactory.getInstance().getTokensIconsLocation();

            for (ErcToken userAsset : portfolioHelper.getUserAssets()) {
                String currentAssetSymbol = userAsset.symbol.toLowerCase(Locale.getDefault());
                Double fiatBalance = Utils.getOrDefault(
                        portfolioHelper.getPerTokenFiatSum(), currentAssetSymbol, 0.0d);
                String fiatBalanceString =
                        String.format(Locale.getDefault(), "$%,.2f", fiatBalance);
                Double cryptoBalance = Utils.getOrDefault(
                        portfolioHelper.getPerTokenCryptoSum(), currentAssetSymbol, 0.0d);
                String cryptoBalanceString = String.format(
                        Locale.getDefault(), "%.4f %s", cryptoBalance, userAsset.symbol);

                WalletListItemModel walletListItemModel =
                        new WalletListItemModel(R.drawable.ic_eth, userAsset.name, userAsset.symbol,
                                // Amount in USD
                                fiatBalanceString,
                                // Amount in current crypto currency/token
                                cryptoBalanceString);

                walletListItemModel.setIconPath("file://" + tokensPath + "/" + userAsset.logo);
                walletListItemModel.setErcToken(userAsset);
                walletListItemModelList.add(walletListItemModel);
            }

            walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
            walletCoinAdapter.setOnWalletListItemClick(AccountDetailActivity.this);
            walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);
            rvAssets.setAdapter(walletCoinAdapter);
            rvAssets.setLayoutManager(new LinearLayoutManager(this));
        });
    }

    private void fetchAccountInfo(String chainId) {
        assert mKeyringController != null;
        mKeyringController.getDefaultKeyringInfo(keyringInfo -> {
            if (keyringInfo == null) {
                return;
            }
            for (AccountInfo accountInfo : keyringInfo.accountInfos) {
                if (accountInfo.address.equals(mAddress) && accountInfo.name.equals(mName)) {
                    AccountInfo[] accountInfos = new AccountInfo[1];
                    accountInfos[0] = accountInfo;
                    Utils.setUpTransactionList(accountInfos, mAssetRatioController,
                            mEthTxController, mErcTokenRegistry, null, null, 0,
                            findViewById(R.id.rv_transactions), this, this, chainId);
                    break;
                }
            }
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
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void onAssetClick(ErcToken asset) {
        Utils.openAssetDetailsActivity(AccountDetailActivity.this, asset.symbol, asset.name,
                asset.contractAddress, asset.logo, asset.decimals);
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        Utils.openTransaction(txInfo, mEthJsonRpcController, this);
    }

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

    @Override
    public void onUserInteraction() {
        if (mKeyringController == null) {
            return;
        }
        mKeyringController.notifyUserInteraction();
    }

    private AccountInfo getThisAccountInfo() {
        AccountInfo accountInfo = new AccountInfo();
        accountInfo.address = mAddress;
        accountInfo.name = mName;
        accountInfo.isImported = mIsImported;
        return accountInfo;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mEthJsonRpcController.close();
        mAssetRatioController.close();
        mBraveWalletService.close();
        mKeyringController.close();
        mEthTxController.close();
        mErcTokenRegistry.close();
        mEthJsonRpcController = null;
        mAssetRatioController = null;
        mBraveWalletService = null;
        mKeyringController = null;
        mEthTxController = null;
        mErcTokenRegistry = null;
        InitEthJsonRpcController();
        InitAssetRatioController();
        InitBraveWalletService();
        InitKeyringController();
        InitErcTokenRegistry();
        InitEthTxController();
    }

    private void InitErcTokenRegistry() {
        if (mErcTokenRegistry != null) {
            return;
        }

        mErcTokenRegistry = ERCTokenRegistryFactory.getInstance().getERCTokenRegistry(this);
    }

    private void InitEthTxController() {
        if (mEthTxController != null) {
            return;
        }

        mEthTxController = EthTxControllerFactory.getInstance().getEthTxController(this);
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }

    private void InitEthJsonRpcController() {
        if (mEthJsonRpcController != null) {
            return;
        }

        mEthJsonRpcController =
                EthJsonRpcControllerFactory.getInstance().getEthJsonRpcController(this);
    }

    private void InitAssetRatioController() {
        if (mAssetRatioController != null) {
            return;
        }

        mAssetRatioController =
                AssetRatioControllerFactory.getInstance().getAssetRatioController(this);
    }

    private void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    public EthJsonRpcController getEthJsonRpcController() {
        return mEthJsonRpcController;
    }

    public AssetRatioController getAssetRatioController() {
        return mAssetRatioController;
    }

    public BraveWalletService getBraveWalletService() {
        return mBraveWalletService;
    }
}
