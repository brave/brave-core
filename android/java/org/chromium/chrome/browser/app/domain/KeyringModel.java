/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.mojo.system.MojoException;

public class KeyringModel implements KeyringServiceObserver {
    private KeyringService mKeyringService;
    private MutableLiveData<KeyringInfo> _mKeyringInfoLiveData;
    public LiveData<KeyringInfo> mKeyringInfoLiveData;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    public LiveData<AccountInfo> mSelectedAccount;
    private CryptoSharedData mSharedData;
    // Todo: create method to interact with keyring

    public KeyringModel(KeyringService mKeyringService, CryptoSharedData sharedData) {
        this.mKeyringService = mKeyringService;
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
            Integer coinType = mSharedData.getCoinType();
            mKeyringService.getSelectedAccount(coinType, accountAddress -> {
                if (keyringInfo.accountInfos.length > 0) {
                    AccountInfo selectedAccount = keyringInfo.accountInfos[0];
                    for (AccountInfo accountInfo : keyringInfo.accountInfos) {
                        if (accountInfo.address.equals(accountAddress)) {
                            selectedAccount = accountInfo;
                            break;
                        }
                    }
                    _mSelectedAccount.postValue(selectedAccount);
                }
            });
        });
    }

    public void setSelectedAccount(String accountAddress, int coin) {
        mKeyringService.setSelectedAccount(accountAddress, coin, isAccountSelected -> {});
    }

    public KeyringInfo getKeyringInfo() {
        return _mKeyringInfoLiveData.getValue();
    }

    public void resetService(KeyringService keyringService) {
        if (mKeyringService == keyringService) {
            return;
        }
        this.mKeyringService = keyringService;
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
