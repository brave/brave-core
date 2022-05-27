/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.view.MenuItem;

import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.AccountsFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class AccountSelectorActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick {
    private KeyringModel mKeyringModel;
    private RecyclerView mRVNetworkSelector;
    private WalletCoinAdapter mWalletCoinAdapter;
    private AccountInfo[] mAccountInfos;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_selector);
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mKeyringModel = activity.getWalletModel().getKeyringModel();
        }
        assert mKeyringModel != null;
        init();
        onInitialLayoutInflationComplete();
    }

    @SuppressLint("NotifyDataSetChanged")
    private void init() {
        mRVNetworkSelector = findViewById(R.id.rv_account_selector_activity);
        mWalletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.SELECT_ACCOUNTS_LIST);
        mRVNetworkSelector.setAdapter(mWalletCoinAdapter);
        mWalletCoinAdapter.setOnWalletListItemClick(this);
        mWalletCoinAdapter.setOnWalletListItemClick(this);
        mKeyringModel.mKeyringInfoLiveData.observe(this, keyringInfo -> {
            if (keyringInfo != null) {
                mAccountInfos = keyringInfo.accountInfos;
                List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
                for (AccountInfo accountInfo : mAccountInfos) {
                    if (!accountInfo.isImported) {
                        walletListItemModelList.add(
                                new WalletListItemModel(R.drawable.ic_eth, accountInfo.name,
                                        accountInfo.address, null, null, accountInfo.isImported));
                    }
                }

                mWalletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
                mWalletCoinAdapter.notifyDataSetChanged();
            }
        });

        mKeyringModel.mSelectedAccount.observe(this, selectedAccountInfo -> {
            if (selectedAccountInfo != null) {
                mWalletCoinAdapter.updateSelectedNetwork(
                        selectedAccountInfo.name, selectedAccountInfo.address);
            }
        });
        MaterialToolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setOnMenuItemClickListener(item -> {
            Intent intent = new Intent(this, AddAccountActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(intent);
            return true;
        });
    }

    @Override
    public void onAccountClick(WalletListItemModel walletListItemModel) {
        for (AccountInfo accountInfo : mAccountInfos) {
            if (walletListItemModel.getTitle().equals(accountInfo.name)
                    && walletListItemModel.getSubTitle().equals(accountInfo.address)) {
                mKeyringModel.setSelectedAccount(accountInfo.address, accountInfo.coin);
                finish();
            }
        }
    }
}
