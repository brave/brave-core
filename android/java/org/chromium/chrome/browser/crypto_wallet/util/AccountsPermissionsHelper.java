/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.HashSet;

public class AccountsPermissionsHelper {
    private BraveWalletService mBraveWalletService;
    private AccountInfo[] mAccounts;
    private Origin mOrigin;
    private HashSet<AccountInfo> mAccountsWithPermissions;

    public AccountsPermissionsHelper(
            BraveWalletService braveWalletService, AccountInfo[] accounts, Origin origin) {
        assert braveWalletService != null;
        assert accounts != null;
        mBraveWalletService = braveWalletService;
        mOrigin = origin;
        mAccounts = accounts;
        mAccountsWithPermissions = new HashSet<AccountInfo>();
    }

    public HashSet<AccountInfo> getAccountsWithPermissions() {
        return mAccountsWithPermissions;
    }

    public void checkAccounts(Runnable runWhenDone) {
        AsyncUtils.MultiResponseHandler accountsPermissionsMultiResponse =
                new AsyncUtils.MultiResponseHandler(mAccounts.length);
        ArrayList<AsyncUtils.GetHasEthereumPermissionResponseContext> accountsPermissionsContexts =
                new ArrayList<AsyncUtils.GetHasEthereumPermissionResponseContext>();
        for (AccountInfo account : mAccounts) {
            AsyncUtils.GetHasEthereumPermissionResponseContext accountPermissionContext =
                    new AsyncUtils.GetHasEthereumPermissionResponseContext(
                            accountsPermissionsMultiResponse.singleResponseComplete);

            accountsPermissionsContexts.add(accountPermissionContext);

            mBraveWalletService.hasPermission(
                    account.coin, mOrigin, account.address, accountPermissionContext);
        }

        accountsPermissionsMultiResponse.setWhenAllCompletedAction(() -> {
            int currentPos = 0;
            for (AsyncUtils.GetHasEthereumPermissionResponseContext accountPermissionContext :
                    accountsPermissionsContexts) {
                assert mAccounts.length > currentPos;
                currentPos++;
                if (!accountPermissionContext.success) {
                    continue;
                }

                if (accountPermissionContext.hasPermission) {
                    mAccountsWithPermissions.add(mAccounts[currentPos - 1]);
                }
            }
            runWhenDone.run();
        });
    }
}
