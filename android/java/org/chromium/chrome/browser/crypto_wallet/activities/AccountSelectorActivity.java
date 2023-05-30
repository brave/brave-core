/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;

import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AccountKind;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.CreateAccountBottomSheetFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

import java.util.ArrayList;
import java.util.List;

public class AccountSelectorActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick {
    private static final String TAG = "AccountSelector";

    private KeyringModel mKeyringModel;
    private RecyclerView mRVNetworkSelector;
    private WalletCoinAdapter mWalletCoinAdapter;
    private AccountInfo[] mAccountInfos;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_selector);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mKeyringModel = activity.getWalletModel().getKeyringModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation " + e);
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
        if (walletListItemModel.getAccountInfo() == null) {
            return;
        }

        mKeyringModel.setSelectedAccount(walletListItemModel.getAccountInfo());
        finish();
    }

    @SuppressLint("NotifyDataSetChanged")
    private void initAccounts() {
        mKeyringModel.mSelectedAccount.observe(this, selectedAccountInfo -> {
            if (selectedAccountInfo == null) return;

            boolean callDataSetChanged = true;
            if (mAccountInfos != null) {
                for (AccountInfo accountInfo : mAccountInfos) {
                    if (WalletUtils.accountIdsEqual(selectedAccountInfo, accountInfo)) {
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
                    // TODO(apaymyshev): Why I'm not allowed to select imported account?
                    if (accountInfo.accountId.kind != AccountKind.IMPORTED) {
                        walletListItemModelList.add(
                                WalletListItemModel.makeForAccountInfo(accountInfo));
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
