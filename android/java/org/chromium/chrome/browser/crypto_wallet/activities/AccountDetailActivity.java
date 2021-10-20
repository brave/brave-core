/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

import java.util.ArrayList;
import java.util.List;

public class AccountDetailActivity
        extends AsyncInitializationActivity implements OnWalletListItemClick {
    private String mAddress;
    private String mName;
    private boolean mIsImported;
    private TextView mAccountText;
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_detail);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
            mIsImported = getIntent().getBooleanExtra(Utils.ISIMPORTED, false);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle("");

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
                accountDetailsWithQrActivityIntent.putExtra(Utils.ISIMPORTED, mIsImported);
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

        setUpAssetList();
        setUpTransactionList();

        onInitialLayoutInflationComplete();
    }

    private void setUpAssetList() {
        RecyclerView rvAssets = findViewById(R.id.rv_assets);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(R.drawable.ic_eth,
                "Basic Attention Token", "BAT", "$10,810.03", "10,037.9028 BAT"));
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Bitcoin", "BTC", "$12,212.81", "0.431 BTC"));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AccountDetailActivity.this);
        walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);
        rvAssets.setAdapter(walletCoinAdapter);
        rvAssets.setLayoutManager(new LinearLayoutManager(this));
    }

    private void setUpTransactionList() {
        RecyclerView rvTransactions = findViewById(R.id.rv_transactions);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Ledger Nano", "0xA1da***7af1", "$37.92", "0.0009431 ETH"));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AccountDetailActivity.this);
        walletCoinAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        rvTransactions.setAdapter(walletCoinAdapter);
        rvTransactions.setLayoutManager(new LinearLayoutManager(this));
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
    public void onAssetClick() {
        Utils.openAssetDetailsActivity(AccountDetailActivity.this);
    }

    @Override
    public void onTransactionClick() {}

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
}
