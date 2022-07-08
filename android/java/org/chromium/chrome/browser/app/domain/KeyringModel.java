/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class KeyringModel implements KeyringServiceObserver {
    private static final int NO_COIN_TYPE = Integer.MIN_VALUE;

    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    private MutableLiveData<KeyringInfo[]> _mKeyringInfosLiveData;
    public LiveData<KeyringInfo[]> mKeyringInfosLiveData;
    private MutableLiveData<KeyringInfo> _mSelectedCoinKeyringInfoLiveData;
    public LiveData<KeyringInfo> mSelectedCoinKeyringInfoLiveData;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    public LiveData<List<AccountInfo>> mAccountInfos;
    private final MutableLiveData<List<AccountInfo>> _mAccountInfos;
    // Prefer using getSelectedAccountOrAccountPerOrigin, especially for dapps
    public LiveData<AccountInfo> mSelectedAccount;
    private CryptoSharedData mSharedData;
    private AccountsPermissionsHelper mAccountsPermissionsHelper;
    private final Object mLock = new Object();
    private CryptoModelActions mCryptoModelActions;
    private HashMap<Integer, String> mKeyringToCoin;

    public KeyringModel(KeyringService keyringService, CryptoSharedData sharedData,
            BraveWalletService braveWalletService, CryptoModelActions cryptoModelActions) {
        mKeyringToCoin = new HashMap<>();
        mKeyringService = keyringService;
        mBraveWalletService = braveWalletService;
        mSharedData = sharedData;
        _mKeyringInfosLiveData = new MutableLiveData<>(new KeyringInfo[0]);
        mKeyringInfosLiveData = _mKeyringInfosLiveData;
        _mSelectedAccount = new MutableLiveData<>();
        mSelectedAccount = _mSelectedAccount;
        mCryptoModelActions = cryptoModelActions;
        _mSelectedCoinKeyringInfoLiveData = new MutableLiveData<>(null);
        mSelectedCoinKeyringInfoLiveData = _mSelectedCoinKeyringInfoLiveData;
        _mAccountInfos = new MutableLiveData<>(Collections.emptyList());
        mAccountInfos = _mAccountInfos;
        initState();
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.addObserver(this);
        }
    }

    private void update(int coinType) {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.getKeyringInfo(getSelectedCoinKeyringId(coinType),
                    keyringInfo -> { _mSelectedCoinKeyringInfoLiveData.postValue(keyringInfo); });
            mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
                List<AccountInfo> accountInfos = getAccountInfosFromKeyrings(keyringInfos);
                _mAccountInfos.postValue(accountInfos);
                _mKeyringInfosLiveData.postValue(keyringInfos);
                mKeyringService.getSelectedAccount(coinType, accountAddress -> {
                    if (accountAddress != null && !accountAddress.isEmpty()) {
                        AccountInfo selectedAccountInfo = null;
                        for (AccountInfo accountInfo : accountInfos) {
                            if (accountInfo.address.equals(accountAddress)) {
                                selectedAccountInfo = accountInfo;
                                break;
                            }
                        }
                        _mSelectedAccount.postValue(selectedAccountInfo);
                    } else if (accountInfos.size() > 0) {
                        _mSelectedAccount.postValue(accountInfos.get(0));
                    }
                });
            });
        }
    }

    private void update() {
        mBraveWalletService.getSelectedCoin(coinType -> { update(coinType); });
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
        synchronized (mLock) {
            if (mKeyringService == null) {
                return mSelectedAccount;
            }
            _mSelectedAccount.setValue(null);
            mKeyringService.getSelectedAccount(mSharedData.getCoinType(), accountAddress -> {
                if (accountAddress == null) {
                    mKeyringService.getKeyringInfo(
                            getSelectedCoinKeyringId(mSharedData.getCoinType()),
                            keyringInfo -> { updateSelectedAccountPerOriginOrFirst(keyringInfo); });
                } else {
                    update();
                }
            });
        }
        return mSelectedAccount;
    }

    public void setSelectedAccount(String accountAddress, int coin) {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.setSelectedAccount(accountAddress, coin, isAccountSelected -> {
                mBraveWalletService.setSelectedCoin(coin);
                mCryptoModelActions.updateCoinType();
            });
        }
    }

    public KeyringInfo getKeyringInfo() {
        return getSelectedCoinKeyringInfo(mSharedData.getCoinType());
    }

    public void resetService(KeyringService keyringService, BraveWalletService braveWalletService) {
        synchronized (mLock) {
            if (mKeyringService != keyringService) {
                mKeyringService = keyringService;
            }
            if (mBraveWalletService != braveWalletService) {
                mBraveWalletService = braveWalletService;
            }
        }
        if (mKeyringService != null && mBraveWalletService != null) {
            init();
        }
    }

    public void getAccounts(Callbacks.Callback1<AccountInfo[]> callback1) {
        mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
            List<AccountInfo> accountInfos = getAccountInfosFromKeyrings(keyringInfos);
            callback1.call(accountInfos.toArray(new AccountInfo[0]));
        });
    }

    @NonNull
    private List<AccountInfo> getAccountInfosFromKeyrings(KeyringInfo[] keyringInfos) {
        List<AccountInfo> accountInfos = new ArrayList<>();
        for (KeyringInfo keyringInfo : keyringInfos) {
            accountInfos.addAll(Arrays.asList(keyringInfo.accountInfos));
        }
        return accountInfos;
    }

    public void addAccount(String accountName, @CoinType.EnumType int coinType,
            Callbacks.Callback1<Boolean> callback) {
        final AccountInfo[] finalAccountInfos =
                getAccountInfosFromKeyrings(_mKeyringInfosLiveData.getValue())
                        .toArray(new AccountInfo[0]);
        mKeyringService.addAccount(accountName, coinType, result -> {
            if (result) {
                boolean hasNoExistingAccountType = true;
                for (AccountInfo accountInfo : finalAccountInfos) {
                    hasNoExistingAccountType = !(accountInfo.coin == coinType);
                    if (hasNoExistingAccountType) break;
                }
                if (hasNoExistingAccountType) {
                    mKeyringService.getKeyringInfo(
                            getSelectedCoinKeyringId(coinType), updatedKeyringInfo -> {
                                for (AccountInfo accountInfo : updatedKeyringInfo.accountInfos) {
                                    if (accountInfo.coin == coinType) {
                                        setSelectedAccount(accountInfo.address, coinType);
                                        break;
                                    }
                                }
                            });
                }
            }
            mCryptoModelActions.updateCoinType();
            callback.call(result);
        });
    }

    private KeyringInfo getSelectedCoinKeyringInfo(int coinType) {
        String selectedCoinKeyringId = getSelectedCoinKeyringId(coinType);
        for (KeyringInfo keyringInfo : _mKeyringInfosLiveData.getValue()) {
            if (keyringInfo.id.equals(selectedCoinKeyringId)) return keyringInfo;
        }
        return null;
    }

    private String getSelectedCoinKeyringId(int coinType) {
        return mKeyringToCoin.get(coinType);
    }

    private void initState() {
        mKeyringToCoin.put(CoinType.ETH, BraveWalletConstants.DEFAULT_KEYRING_ID);
        mKeyringToCoin.put(CoinType.SOL, BraveWalletConstants.SOLANA_KEYRING_ID);
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
        update(coin);
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}
}
