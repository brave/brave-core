/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.UiThread;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.system.MojoException;

import java.util.HashSet;

public class KeyringModel implements KeyringServiceObserver {
    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    private MutableLiveData<KeyringInfo> _mKeyringInfoLiveData;
    public LiveData<KeyringInfo> mKeyringInfoLiveData;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    public LiveData<AccountInfo> mSelectedAccount;
    private CryptoSharedData mSharedData;
    private AccountsPermissionsHelper mAccountsPermissionsHelper;

    public KeyringModel(KeyringService keyringService, CryptoSharedData sharedData,
            BraveWalletService braveWalletService) {
        mKeyringService = keyringService;
        mBraveWalletService = braveWalletService;
        mSharedData = sharedData;
        _mKeyringInfoLiveData = new MutableLiveData<>(null);
        mKeyringInfoLiveData = _mKeyringInfoLiveData;
        _mSelectedAccount = new MutableLiveData<>();
        mSelectedAccount = _mSelectedAccount;
    }

    public void init() {
        mKeyringService.addObserver(this);
    }

    private void update() {
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
            _mKeyringInfoLiveData.postValue(keyringInfo);

            mKeyringService.getSelectedAccount(mSharedData.getCoinType(), accountAddress -> {
                if (accountAddress != null && !accountAddress.isEmpty()) {
                    AccountInfo selectedAccountInfo = null;
                    for (AccountInfo accountInfo : keyringInfo.accountInfos) {
                        if (accountInfo.address.equals(accountAddress)) {
                            selectedAccountInfo = accountInfo;
                            break;
                        }
                    }
                    _mSelectedAccount.postValue(selectedAccountInfo);
                } else if (keyringInfo.accountInfos.length > 0) {
                    _mSelectedAccount.postValue(keyringInfo.accountInfos[0]);
                }
            });
        });
    }

    private void updateSelectedAccountPerOriginOrFirst(KeyringInfo keyringInfo) {
        mAccountsPermissionsHelper = new AccountsPermissionsHelper(
                mBraveWalletService, keyringInfo.accountInfos, Utils.getCurrentMojomOrigin());
        mAccountsPermissionsHelper.checkAccounts(() -> {
            String selectedAccountAddress = null;
            HashSet<AccountInfo> permissionAccounts =
                    mAccountsPermissionsHelper.getAccountsWithPermissions();
            if (!permissionAccounts.isEmpty()) {
                selectedAccountAddress = permissionAccounts.iterator().next().address;
            } else if (keyringInfo.accountInfos.length > 0) {
                selectedAccountAddress = keyringInfo.accountInfos[0].address;
            }
            if (selectedAccountAddress != null) {
                setSelectedAccount(selectedAccountAddress, mSharedData.getCoinType());
            }
        });
    }

    /**
     * Enforce to fetch and use the first permitted account if there is no selected account in
     * Keyring service
     *
     * @return mSelectedAccount live data to get the selected account
     */
    @UiThread
    public LiveData<AccountInfo> getSelectedAccountOrAccountPerOrigin() {
        _mSelectedAccount.setValue(null);
        mKeyringService.getSelectedAccount(mSharedData.getCoinType(), accountAddress -> {
            if (accountAddress == null) {
                mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                        keyringInfo -> { updateSelectedAccountPerOriginOrFirst(keyringInfo); });
            } else {
                update();
            }
        });
        return mSelectedAccount;
    }

    public void setSelectedAccount(String accountAddress, int coin) {
        mKeyringService.setSelectedAccount(accountAddress, coin, isAccountSelected -> {});
    }

    public KeyringInfo getKeyringInfo() {
        return _mKeyringInfoLiveData.getValue();
    }

    public void resetService(KeyringService keyringService, BraveWalletService braveWalletService) {
        if (mKeyringService != keyringService) {
            mKeyringService = keyringService;
        }
        if (mBraveWalletService != braveWalletService) {
            mBraveWalletService = braveWalletService;
        }
        init();
    }

    @Override
    public void keyringCreated(String keyringId) {
        update();
    }

    @Override
    public void keyringRestored(String keyringId) {
        update();
    }

    @Override
    public void keyringReset() {
        update();
    }

    @Override
    public void locked() {
        update();
    }

    @Override
    public void unlocked() {
        update();
    }

    @Override
    public void backedUp() {
        update();
    }

    @Override
    public void accountsChanged() {
        update();
    }

    @Override
    public void autoLockMinutesChanged() {}

    @Override
    public void selectedAccountChanged(int coin) {
        update();
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}
}
