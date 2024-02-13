/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

public class AccountDetailActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick, ApprovedTxObserver {
    private static final String TAG = "AccountDetail";

    private AccountInfo mAccountInfo;
    private TextView mAccountText;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private WalletCoinAdapter mWalletTxCoinAdapter;
    private WalletModel mWalletModel;

    public static Intent createIntent(@NonNull Context context, @NonNull AccountInfo accountInfo) {
        Intent intent = new Intent(context, AccountDetailActivity.class);
        WalletUtils.addAccountInfoToIntent(intent, accountInfo);
        return intent;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_detail);

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        if (getIntent() != null) {
            mAccountInfo = WalletUtils.getAccountInfoFromIntent(getIntent());
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle("");

        ImageView accountPicture = findViewById(R.id.account_picture);
        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, accountPicture, mAccountInfo, true);

        mAccountText = findViewById(R.id.account_text);
        mAccountText.setText(mAccountInfo.name);

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText(Utils.stripAccountAddress(mAccountInfo.address));

        TextView btnDetails = findViewById(R.id.details_btn);
        btnDetails.setOnClickListener(v -> {
            Intent intent = AccountDetailsWithQrActivity.createIntent(
                    AccountDetailActivity.this, mAccountInfo);
            startActivity(intent);
        });
        TextView btnRename = findViewById(R.id.rename_btn);
        btnRename.setOnClickListener(v -> {
            Intent intent = AddAccountActivity.createIntentToEditAccount(this, mAccountInfo);
            startActivityForResult(intent, Utils.ACCOUNT_REQUEST_CODE);
        });

        onInitialLayoutInflationComplete();
    }

    private void setUpAssetList() {
        AccountInfo[] accountInfos = new AccountInfo[] {mAccountInfo};
        if (mWalletModel == null) return;
        LiveDataUtil.observeOnce(
                getNetworkModel().mCryptoNetworks, allNetworks -> {
                    PortfolioHelper portfolioHelper =
                            new PortfolioHelper(this, allNetworks, accountInfos);
                    portfolioHelper.setSelectedNetworks(
                            allNetworks.stream()
                                    .filter(networkInfo
                                            -> networkInfo.coin == mAccountInfo.accountId.coin)
                                    .collect(Collectors.toList()));
                    portfolioHelper.fetchAssetsAndDetails(
                            TokenUtils.TokenType.ALL, portfolioHelperCopy -> {
                                RecyclerView rvAssets = findViewById(R.id.rv_assets);

                                List<BlockchainToken> userAssets = portfolioHelper.getUserAssets();
                                HashMap<String, Double> perTokenCryptoSum =
                                        portfolioHelper.getPerTokenCryptoSum();
                                HashMap<String, Double> perTokenFiatSum =
                                        portfolioHelper.getPerTokenFiatSum();

                                String tokensPath = BlockchainRegistryFactory.getInstance()
                                                            .getTokensIconsLocation();

                                WalletCoinAdapter walletCoinAdapter = Utils.setupVisibleAssetList(
                                        userAssets, perTokenCryptoSum, perTokenFiatSum, tokensPath,
                                        getResources(), allNetworks);
                                walletCoinAdapter.setOnWalletListItemClick(
                                        AccountDetailActivity.this);
                                rvAssets.setAdapter(walletCoinAdapter);
                                rvAssets.setLayoutManager(new LinearLayoutManager(this));
                            });
                });
    }

    // TODO(apaymyshev): this should not depend on selectedNetwork
    private void fetchAccountInfo(NetworkInfo selectedNetwork) {
        AccountInfo[] accounts = {mAccountInfo};
        LiveDataUtil.observeOnce(
                getNetworkModel().mCryptoNetworks, allNetworks -> {
                    Utils.getTxExtraInfo(new WeakReference<>(this), TokenUtils.TokenType.ALL,
                            allNetworks, selectedNetwork, accounts, null, false,
                            (assetPrices, fullTokenList, nativeAssetsBalances,
                                    blockchainTokensBalances) -> {
                                WalletListItemModel thisAccountItemModel =
                                        WalletListItemModel.makeForAccountInfo(mAccountInfo);
                                Utils.setUpTransactionList(this, accounts, allNetworks,
                                        thisAccountItemModel, assetPrices, fullTokenList,
                                        nativeAssetsBalances, blockchainTokensBalances,
                                        selectedNetwork, walletListItemModelList -> {
                                            showTransactionList(walletListItemModelList);
                                        });
                            });
                });
    }

    private void showTransactionList(List<WalletListItemModel> walletListItemModelList) {
        mWalletTxCoinAdapter.setWalletCoinAdapterType(
                WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);

        mWalletTxCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        mWalletTxCoinAdapter.setOnWalletListItemClick(AccountDetailActivity.this);
        mWalletTxCoinAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        RecyclerView rvTransactions = findViewById(R.id.rv_transactions);
        rvTransactions.setAdapter(mWalletTxCoinAdapter);
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
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "finishNativeInitialization " + e);
        }
        assert mJsonRpcService != null && mWalletModel != null;
        setUpAssetList();
        LiveDataUtil.observeOnce(getNetworkModel().mDefaultNetwork,
                selectedNetwork -> { fetchAccountInfo(selectedNetwork); });
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        Utils.openAssetDetailsActivity(AccountDetailActivity.this, asset);
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        if (mWalletModel == null) return;
        NetworkInfo txNetwork = getNetworkModel().getNetwork(txInfo.chainId);
        Utils.openTransaction(txInfo, this, txNetwork.coin, txNetwork);
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String txId) {}

    @Override
    public void onTxPending(String txId) {}

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == Utils.ACCOUNT_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK && data != null) {
                String name = data.getStringExtra(Utils.NAME);
                if (name != null) {
                    mAccountInfo.name = name;
                    mAccountText.setText(name);
                }
            }
        }
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();

        Intent returnIntent = new Intent();
        setResult(Activity.RESULT_OK, returnIntent);
        finish();
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        mWalletTxCoinAdapter.onTransactionUpdate(txInfo);
    }

    private void initState() {
        mWalletTxCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
    }

    private NetworkModel getNetworkModel() {
        return mWalletModel.getNetworkModel();
    }

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}
