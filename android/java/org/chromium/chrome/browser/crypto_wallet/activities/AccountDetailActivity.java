/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

import java.util.ArrayList;
import java.util.List;

public class AccountDetailActivity
        extends AsyncInitializationActivity implements OnWalletListItemClick {
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_detail);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle("");

        TextView accountText = findViewById(R.id.account_text);
        accountText.setText("Account 2");

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText("0xffd3...32ee");

        TextView btnDetails = findViewById(R.id.details_btn);
        btnDetails.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent accountDetailsWithQrActivityIntent =
                        new Intent(AccountDetailActivity.this, AccountDetailsWithQrActivity.class);
                startActivity(accountDetailsWithQrActivityIntent);
            }
        });
        TextView btnEdit = findViewById(R.id.edit_btn);
        btnEdit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO add action for edit
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
}
