/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;

public class SelectedAccountResponsesCollector {
    private KeyringService mKeyringService;
    private List<Integer> mCoinTypes;
    private HashSet<AccountInfo> mSelectedAccountsPerCoin;
    private List<AccountInfo> mAllAccountInfos;

    public SelectedAccountResponsesCollector(KeyringService keyringService, List<Integer> coinTypes,
            List<AccountInfo> allAccountInfos) {
        assert keyringService != null;
        mKeyringService = keyringService;
        mCoinTypes = coinTypes;
        mAllAccountInfos = allAccountInfos;
        mSelectedAccountsPerCoin = new LinkedHashSet<>();
    }

    public void getAccounts(Callbacks.Callback1<HashSet<AccountInfo>> runWhenDone) {
        AsyncUtils.MultiResponseHandler selectedAccountInfosPerCoinMultiResponse =
                new AsyncUtils.MultiResponseHandler(mCoinTypes.size());
        ArrayList<AsyncUtils.GetSelectedAccountResponseContext> accountsPermissionsContexts =
                new ArrayList<>();
        for (int coin : mCoinTypes) {
            AsyncUtils.GetSelectedAccountResponseContext accountContext =
                    new AsyncUtils.GetSelectedAccountResponseContext(
                            selectedAccountInfosPerCoinMultiResponse.singleResponseComplete, coin);

            accountsPermissionsContexts.add(accountContext);
            if (CoinType.FIL != coin) {
                mKeyringService.getSelectedAccount(coin, accountContext);
            }
        }

        selectedAccountInfosPerCoinMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetSelectedAccountResponseContext getSelectedAccountResponseContext :
                    accountsPermissionsContexts) {
                if (TextUtils.isEmpty(getSelectedAccountResponseContext.selectedAccount)) {
                    continue;
                }
                AccountInfo selectedAccountInfo = null;
                for (AccountInfo accountInfo : mAllAccountInfos) {
                    if (accountInfo.address.equals(
                                getSelectedAccountResponseContext.selectedAccount)) {
                        selectedAccountInfo = accountInfo;
                        break;
                    }
                }
                if (selectedAccountInfo != null) {
                    mSelectedAccountsPerCoin.add(selectedAccountInfo);
                }
            }
            runWhenDone.call(mSelectedAccountsPerCoin);
        });
    }
}
