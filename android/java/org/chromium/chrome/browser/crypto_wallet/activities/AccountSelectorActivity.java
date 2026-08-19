/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;

import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AccountKind;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.AccountSelectorRecyclerView;
import org.chromium.chrome.browser.crypto_wallet.listeners.AccountSelectorItemListener;
import org.chromium.chrome.browser.crypto_wallet.model.AccountSelectorItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

import java.util.ArrayList;
import java.util.List;

@NullMarked
public class AccountSelectorActivity extends BraveWalletBaseActivity
        implements AccountSelectorItemListener {
    private static final String TAG = "AccountSelector";

    private @MonotonicNonNull KeyringModel mKeyringModel;
    private AccountSelectorRecyclerView mAccountSelectorRecyclerView;
    private AccountInfo @Nullable [] mAccountInfos;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_selector);

        final RecyclerView accountSelector = findViewById(R.id.rv_account_selector_activity);
        mAccountSelectorRecyclerView = new AccountSelectorRecyclerView();
        accountSelector.setAdapter(mAccountSelectorRecyclerView);
        mAccountSelectorRecyclerView.setAccountSelectorItemListener(this);

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            WalletModel walletModel = activity.getWalletModel();
            if (walletModel != null) {
                mKeyringModel = walletModel.getKeyringModel();
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "finishNativeInitialization", e);
        }

        initAccounts();
    }

    @Override
    public void onAccountClick(final AccountSelectorItemModel accountSelectorItemModel) {
        final AccountInfo accountInfo = accountSelectorItemModel.getAccountInfo();
        if (mKeyringModel == null || accountInfo == null) {
            return;
        }

        mKeyringModel.setSelectedAccount(accountInfo);
        finish();
    }

    @SuppressLint("NotifyDataSetChanged")
    private void initAccounts() {
        if (mKeyringModel == null) {
            return;
        }
        mKeyringModel.mSelectedAccount.observe(
                this,
                selectedAccountInfo -> {
                    if (selectedAccountInfo == null) return;

                    boolean callDataSetChanged = true;
                    final AccountInfo[] currentAccountInfos = mAccountInfos;
                    if (currentAccountInfos != null) {
                        for (AccountInfo accountInfo : currentAccountInfos) {
                            if (WalletUtils.accountIdsEqual(selectedAccountInfo, accountInfo)) {
                                callDataSetChanged = false;
                                break;
                            }
                        }
                    }
                    if (!callDataSetChanged) return;

                    mKeyringModel.getAccounts(
                            accountInfos -> {
                                mAccountInfos = accountInfos;
                                List<AccountSelectorItemModel> accountSelectorItemModelList =
                                        new ArrayList<>();
                                for (AccountInfo accountInfo : accountInfos) {
                                    // TODO(apaymyshev): Why I'm not allowed to select imported
                                    // account?
                                    if (accountInfo.accountId.kind != AccountKind.IMPORTED
                                            && WalletConstants.SUPPORTED_COIN_TYPES_ON_DAPPS
                                                    .contains(accountInfo.accountId.coin)) {
                                        accountSelectorItemModelList.add(
                                                AccountSelectorItemModel.makeForAccountInfo(
                                                        accountInfo));
                                    }
                                }

                                mAccountSelectorRecyclerView.setWalletListItemModelList(
                                        accountSelectorItemModelList);
                                mAccountSelectorRecyclerView.notifyDataSetChanged();
                                mAccountSelectorRecyclerView.updateSelectedNetwork(
                                        selectedAccountInfo.name, selectedAccountInfo.address);
                            });
                });
    }
}
