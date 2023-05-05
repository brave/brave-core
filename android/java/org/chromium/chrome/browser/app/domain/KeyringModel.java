/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import androidx.annotation.UiThread;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.SelectedAccountResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class KeyringModel implements KeyringServiceObserver {
    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    @FilecoinNetworkType
    private String mSelectedFilecoinNetwork;
    private MutableLiveData<KeyringInfo[]> _mKeyringInfosLiveData;
    private MutableLiveData<KeyringInfo> _mSelectedCoinKeyringInfoLiveData;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    private final MutableLiveData<List<AccountInfo>> _mAccountInfos;
    // Prefer using getSelectedAccountOrAccountPerOrigin, especially for dapps
    private CryptoSharedData mSharedData;
    private AccountsPermissionsHelper mAccountsPermissionsHelper;
    private final Object mLock = new Object();
    private CryptoSharedActions mCryptoSharedActions;
    private MediatorLiveData<Pair<AccountInfo, List<AccountInfo>>> _mAccountAllAccountsPair;
    public LiveData<Pair<AccountInfo, List<AccountInfo>>> mAccountAllAccountsPair;
    public LiveData<List<AccountInfo>> mAccountInfos;
    public LiveData<KeyringInfo> mSelectedCoinKeyringInfoLiveData;
    public LiveData<AccountInfo> mSelectedAccount;
    public LiveData<KeyringInfo[]> mKeyringInfosLiveData;

    public KeyringModel(KeyringService keyringService, CryptoSharedData sharedData,
            BraveWalletService braveWalletService, CryptoSharedActions cryptoSharedActions) {
        mKeyringService = keyringService;
        mBraveWalletService = braveWalletService;
        mSharedData = sharedData;
        _mKeyringInfosLiveData = new MutableLiveData<>(new KeyringInfo[0]);
        mKeyringInfosLiveData = _mKeyringInfosLiveData;
        _mSelectedAccount = new MutableLiveData<>();
        mSelectedAccount = _mSelectedAccount;
        mCryptoSharedActions = cryptoSharedActions;
        _mSelectedCoinKeyringInfoLiveData = new MutableLiveData<>(null);
        mSelectedCoinKeyringInfoLiveData = _mSelectedCoinKeyringInfoLiveData;
        _mAccountInfos = new MutableLiveData<>(Collections.emptyList());
        mAccountInfos = _mAccountInfos;
        _mAccountAllAccountsPair = new MediatorLiveData<>();
        mAccountAllAccountsPair = _mAccountAllAccountsPair;

        mSelectedFilecoinNetwork = BraveWalletConstants.FILECOIN_MAINNET;

        _mAccountAllAccountsPair.addSource(_mSelectedAccount, accountInfo -> {
            _mAccountAllAccountsPair.postValue(Pair.create(accountInfo, _mAccountInfos.getValue()));
        });
        _mAccountAllAccountsPair.addSource(_mAccountInfos, accountInfos -> {
            _mAccountAllAccountsPair.postValue(
                    Pair.create(_mSelectedAccount.getValue(), accountInfos));
        });
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.addObserver(this);
        }
    }

    public void getDefaultAccountPerCoin(
            Callbacks.Callback1<Set<AccountInfo>> defaultAccountPerCoins) {
        synchronized (mLock) {
            if (mKeyringService == null || mBraveWalletService == null) {
                return;
            }
            List<Integer> coins = new ArrayList<>();
            for (CryptoAccountTypeInfo cryptoAccountTypeInfo :
                    mSharedData.getSupportedCryptoAccountTypes()) {
                coins.add(cryptoAccountTypeInfo.getCoinType());
            }
            mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
                List<AccountInfo> accountInfos =
                        WalletUtils.getAccountInfosFromKeyrings(keyringInfos);
                new SelectedAccountResponsesCollector(
                        mKeyringService, mBraveWalletService, coins, accountInfos)
                        .getAccounts(defaultAccountPerCoin -> {
                            defaultAccountPerCoins.call(defaultAccountPerCoin);
                        });
            });
        }
    }

    private void update(int coinType, String chainId) {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.getKeyringInfo(getSelectedCoinKeyringId(coinType, chainId),
                    keyringInfo -> { _mSelectedCoinKeyringInfoLiveData.postValue(keyringInfo); });
            mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
                final List<AccountInfo> accountInfos =
                        WalletUtils.getAccountInfosFromKeyrings(keyringInfos);
                _mAccountInfos.postValue(accountInfos);
                _mKeyringInfosLiveData.postValue(keyringInfos);
                if (coinType == CoinType.FIL) {
                    mKeyringService.getFilecoinSelectedAccount(chainId, accountAddress -> {
                        updateSelectedAccountAndState(accountAddress, accountInfos);
                    });
                } else {
                    mKeyringService.getSelectedAccount(coinType, accountAddress -> {
                        updateSelectedAccountAndState(accountAddress, accountInfos);
                    });
                }
            });
        }
    }

    void update() {
        if (mBraveWalletService != null) {
            mBraveWalletService.getSelectedCoin(coinType -> {
                mBraveWalletService.getChainIdForActiveOrigin(
                        coinType, chainId -> { update(coinType, chainId); });
            });
        }
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

    private void updateSelectedAccountAndState(
            String accountAddress, List<AccountInfo> accountInfoList) {
        if (!TextUtils.isEmpty(accountAddress)) {
            AccountInfo selectedAccountInfo = null;
            for (AccountInfo accountInfo : accountInfoList) {
                if (accountInfo.address.equals(accountAddress)) {
                    selectedAccountInfo = accountInfo;
                    break;
                }
            }
            _mSelectedAccount.postValue(selectedAccountInfo);
        } else if (accountInfoList.size() > 0) {
            AccountInfo accountInfo = accountInfoList.get(0);
            _mSelectedAccount.postValue(accountInfo);
            setSelectedAccount(accountInfo.address, accountInfo.coin);
        }
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
            @CoinType.EnumType
            int coinType = mSharedData.getCoinType();
            if (coinType == CoinType.FIL) {
                mKeyringService.getFilecoinSelectedAccount(mSelectedFilecoinNetwork,
                        accountAddress -> { updateSelectedAccountAndState(accountAddress); });
            } else {
                mKeyringService.getSelectedAccount(coinType,
                        accountAddress -> { updateSelectedAccountAndState(accountAddress); });
            }
        }
        return mSelectedAccount;
    }

    public void setSelectedAccount(String accountAddress, int coin) {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            AccountInfo selectedAccount = _mSelectedAccount.getValue();
            if (selectedAccount != null && selectedAccount.address.equals(accountAddress)
                    && selectedAccount.coin == coin)
                return;
            mKeyringService.setSelectedAccount(accountAddress, coin, isAccountSelected -> {
                mBraveWalletService.setSelectedCoin(coin);
                mCryptoSharedActions.updateCoinType();
            });
        }
    }

    public void getKeyringInfo(Callback callback) {
        mBraveWalletService.getChainIdForActiveOrigin(mSharedData.getCoinType(), chainId -> {
            callback.onKeyringInfoReady(
                    getSelectedCoinKeyringInfo(mSharedData.getCoinType(), chainId));
        });
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

    public List<AccountInfo> stripNoSwapSupportedAccounts(List<AccountInfo> accountInfos) {
        List<AccountInfo> accountInfosFiltered = new ArrayList<>();
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.coin != CoinType.SOL) {
                accountInfosFiltered.add(accountInfo);
            }
        }

        return accountInfosFiltered;
    }

    public void getKeyringPerId(String keyringId, Callbacks.Callback1<KeyringInfo> callback) {
        if (TextUtils.isEmpty(keyringId)) return;
        KeyringInfo selectedCoinKeyring = _mSelectedCoinKeyringInfoLiveData.getValue();
        if (selectedCoinKeyring != null && selectedCoinKeyring.id.equals(keyringId)) {
            callback.call(selectedCoinKeyring);
        } else {
            if (mKeyringService == null) {
                callback.call(null);
                return;
            }
            mKeyringService.getKeyringInfo(
                    keyringId, keyringInfo -> { callback.call(keyringInfo); });
        }
    }

    public void getAccounts(Callbacks.Callback1<AccountInfo[]> callback1) {
        mKeyringService.getKeyringsInfo(mSharedData.getEnabledKeyrings(), keyringInfos -> {
            List<AccountInfo> accountInfos = WalletUtils.getAccountInfosFromKeyrings(keyringInfos);
            callback1.call(accountInfos.toArray(new AccountInfo[0]));
        });
    }

    public void addAccount(String accountName, @CoinType.EnumType int coinType,
            Callbacks.Callback1<Boolean> callback) {
        mKeyringService.addAccount(
                accountName, coinType, result -> { handleAddAccountResult(result, callback); });
    }

    public void addFilecoinAccount(String accountName, @FilecoinNetworkType String filecoinNetwork,
            Callbacks.Callback1<Boolean> callback) {
        mSelectedFilecoinNetwork = filecoinNetwork;
        mKeyringService.addFilecoinAccount(accountName, mSelectedFilecoinNetwork,
                result -> { handleAddAccountResult(result, callback); });
    }

    public void isWalletLocked(Callbacks.Callback1<Boolean> callback) {
        mKeyringService.isLocked(isWalletLocked -> callback.call(isWalletLocked));
    }

    private void handleAddAccountResult(boolean result, Callbacks.Callback1<Boolean> callback) {
        mCryptoSharedActions.updateCoinType();
        mCryptoSharedActions.onNewAccountAdded();
        callback.call(result);
    }

    private void updateSelectedAccountAndState(@Nullable String accountAddress) {
        if (accountAddress == null) {
            mKeyringService.getKeyringInfo(
                    getSelectedCoinKeyringId(mSharedData.getCoinType(), mSharedData.getChainId()),
                    keyringInfo -> { updateSelectedAccountPerOriginOrFirst(keyringInfo); });
        } else {
            update();
        }
    }

    private KeyringInfo getSelectedCoinKeyringInfo(
            @CoinType.EnumType int coinType, @Nullable String chainId) {
        String selectedCoinKeyringId = getSelectedCoinKeyringId(coinType, chainId);
        for (KeyringInfo keyringInfo : _mKeyringInfosLiveData.getValue()) {
            if (keyringInfo.id.equals(selectedCoinKeyringId)) return keyringInfo;
        }
        return null;
    }

    private String getSelectedCoinKeyringId(
            @CoinType.EnumType int coinType, @Nullable String chainId) {
        return AssetUtils.getKeyring(coinType, chainId);
    }

    public interface Callback {
        void onKeyringInfoReady(KeyringInfo keyringInfo);
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
    public void accountsAdded(int coin, String[] addresses) {}

    @Override
    public void autoLockMinutesChanged() {}

    @Override
    public void selectedAccountChanged(int coin) {
        mCryptoSharedActions.updateCoinAccountNetworkInfo(coin);
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}

    @StringDef({BraveWalletConstants.FILECOIN_MAINNET, BraveWalletConstants.FILECOIN_TESTNET})
    public @interface FilecoinNetworkType {}
}
