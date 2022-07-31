/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;

import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.CreateAccountBottomSheetFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;

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

    private void init() {
        mRVNetworkSelector = findViewById(R.id.rv_account_selector_activity);
        mWalletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.SELECT_ACCOUNTS_LIST);
        mRVNetworkSelector.setAdapter(mWalletCoinAdapter);
        mWalletCoinAdapter.setOnWalletListItemClick(this);
        initAccounts();

        MaterialToolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setOnMenuItemClickListener(item -> {
            BottomSheetDialogFragment sheetDialogFragment = new CreateAccountBottomSheetFragment();
            sheetDialogFragment.show(
                    getSupportFragmentManager(), CreateAccountBottomSheetFragment.TAG);
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

    @SuppressLint("NotifyDataSetChanged")
    private void initAccounts() {
        mKeyringModel.mSelectedAccount.observe(this, selectedAccountInfo -> {
            if (selectedAccountInfo == null) return;

            boolean callDataSetChanged = true;
            if (mAccountInfos != null) {
                for (AccountInfo accountInfo : mAccountInfos) {
                    if (selectedAccountInfo.address.equals(accountInfo.address)) {
                        callDataSetChanged = false;
                        break;
                    }
                }
            }
            if (!callDataSetChanged) return;

            mKeyringModel.getAccounts(accountInfos -> {
                mAccountInfos = accountInfos;
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
                mWalletCoinAdapter.updateSelectedNetwork(
                        selectedAccountInfo.name, selectedAccountInfo.address);
            });
        });
    }
}
