/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;
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
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringId;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.SelectedAccountResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class KeyringModel implements KeyringServiceObserver {
    private Context mContext;
    private KeyringService mKeyringService;
    private JsonRpcService mJsonRpcService;
    private BraveWalletService mBraveWalletService;
    @FilecoinNetworkType
    private String mSelectedFilecoinNetwork;
    private MutableLiveData<KeyringInfo> _mSelectedCoinKeyringInfoLiveData;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    private final MutableLiveData<List<AccountInfo>> _mAccountInfos;
    // Prefer using getSelectedAccountOrAccountPerOrigin, especially for dapps
    private CryptoSharedData mSharedData;
    private AccountsPermissionsHelper mAccountsPermissionsHelper;
    private final Object mLock = new Object();
    private CryptoSharedActions mCryptoSharedActions;
    private MediatorLiveData<Pair<AccountInfo, List<AccountInfo>>> _mAccountAllAccountsPair;
    private Origin mOrigin;
    public LiveData<Pair<AccountInfo, List<AccountInfo>>> mAccountAllAccountsPair;
    public LiveData<List<AccountInfo>> mAccountInfos;
    public LiveData<KeyringInfo> mSelectedCoinKeyringInfoLiveData;
    public LiveData<AccountInfo> mSelectedAccount;

    public KeyringModel(Context context, KeyringService keyringService,
            JsonRpcService jsonRpcService, CryptoSharedData sharedData,
            BraveWalletService braveWalletService, CryptoSharedActions cryptoSharedActions) {
        mContext = context;
        mKeyringService = keyringService;
        mJsonRpcService = jsonRpcService;
        mBraveWalletService = braveWalletService;
        mSharedData = sharedData;
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
            mKeyringService.getAllAccounts(allAccounts -> {
                new SelectedAccountResponsesCollector(mKeyringService, mJsonRpcService, coins,
                        Arrays.asList(allAccounts.accounts))
                        .getAccounts(mOrigin, defaultAccountPerCoin -> {
                            defaultAccountPerCoins.call(defaultAccountPerCoin);
                        });
            });
        }
    }

    void setOrigin(Origin origin) {
        mOrigin = origin;
    }

    private void update(int coinType, String chainId) {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.getKeyringInfo(getSelectedCoinKeyringId(coinType, chainId),
                    keyringInfo -> { _mSelectedCoinKeyringInfoLiveData.postValue(keyringInfo); });
            mKeyringService.getAllAccounts(allAccounts -> {
                final List<AccountInfo> accountInfos = Arrays.asList(allAccounts.accounts);
                _mAccountInfos.postValue(accountInfos);
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
                mJsonRpcService.getNetwork(coinType, mOrigin,
                        networkInfo -> { update(coinType, networkInfo.chainId); });
            });
        }
    }

    private void updateSelectedAccountPerOriginOrFirst(KeyringInfo keyringInfo) {
        mAccountsPermissionsHelper = new AccountsPermissionsHelper(
                mBraveWalletService, keyringInfo.accountInfos, Utils.getCurrentMojomOrigin());
        mAccountsPermissionsHelper.checkAccounts(() -> {
            AccountInfo selectedAccount = null;
            HashSet<AccountInfo> permissionAccounts =
                    mAccountsPermissionsHelper.getAccountsWithPermissions();
            if (!permissionAccounts.isEmpty()) {
                selectedAccount = permissionAccounts.iterator().next();
            } else if (keyringInfo.accountInfos.length > 0) {
                selectedAccount = keyringInfo.accountInfos[0];
            }
            if (selectedAccount != null) {
                setSelectedAccount(selectedAccount);
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
            setSelectedAccount(accountInfo);
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

    public void setSelectedAccount(AccountInfo accountInfoToSelect) {
        synchronized (mLock) {
            if (mKeyringService == null || accountInfoToSelect == null) {
                return;
            }
            AccountInfo selectedAccount = _mSelectedAccount.getValue();
            if (selectedAccount != null
                    && WalletUtils.accountIdsEqual(selectedAccount, accountInfoToSelect)) {
                return;
            }

            mKeyringService.setSelectedAccount(accountInfoToSelect.accountId,
                    isAccountSelected -> { mCryptoSharedActions.updateCoinType(); });
        }
    }

    public void getKeyringInfo(Callback callback) {
        mJsonRpcService.getNetwork(mSharedData.getCoinType(), mOrigin, networkInfo -> {
            @KeyringId.EnumType
            int keyringId =
                    getSelectedCoinKeyringId(mSharedData.getCoinType(), networkInfo.chainId);
            mKeyringService.getKeyringInfo(
                    keyringId, keyringInfo -> { callback.onKeyringInfoReady(keyringInfo); });
        });
    }

    public void resetService(Context context, KeyringService keyringService,
            BraveWalletService braveWalletService, JsonRpcService jsonRpcService) {
        synchronized (mLock) {
            mContext = context;
            mKeyringService = keyringService;
            mBraveWalletService = braveWalletService;
            mJsonRpcService = jsonRpcService;
        }
        if (mKeyringService != null && mBraveWalletService != null) {
            init();
        }
    }

    public void getKeyringPerId(
            @KeyringId.EnumType int keyringId, Callbacks.Callback1<KeyringInfo> callback) {
        KeyringInfo selectedCoinKeyring = _mSelectedCoinKeyringInfoLiveData.getValue();
        if (selectedCoinKeyring != null && selectedCoinKeyring.id == keyringId) {
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
        mKeyringService.getAllAccounts(allAccounts -> { callback1.call(allAccounts.accounts); });
    }

    private void addAccountInternal(@CoinType.EnumType int coinType,
            @KeyringId.EnumType int keyringId, String accountName,
            Callbacks.Callback1<Boolean> callback) {
        mKeyringService.addAccount(coinType, keyringId, accountName,
                result -> { handleAddAccountResult(result, callback); });
    }

    public void addAccount(@CoinType.EnumType int coinType, String chainId, String accountName,
            Callbacks.Callback1<Boolean> callback) {
        @KeyringId.EnumType
        int keyringId = AssetUtils.getKeyring(coinType, chainId);
        if (accountName == null) {
            LiveDataUtil.observeOnce(mAccountInfos, accounts -> {
                addAccountInternal(coinType, keyringId,
                        WalletUtils.generateUniqueAccountName(
                                mContext, coinType, accounts.toArray(new AccountInfo[0])),
                        callback);
            });
        } else {
            addAccountInternal(coinType, keyringId, accountName, callback);
        }
    }

    public void isWalletLocked(Callbacks.Callback1<Boolean> callback) {
        mKeyringService.isLocked(isWalletLocked -> callback.call(isWalletLocked));
    }

    private void handleAddAccountResult(AccountInfo result, Callbacks.Callback1<Boolean> callback) {
        mCryptoSharedActions.updateCoinType();
        mCryptoSharedActions.onNewAccountAdded();
        callback.call(result != null);
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

    private @KeyringId.EnumType int getSelectedCoinKeyringId(
            @CoinType.EnumType int coinType, @Nullable String chainId) {
        return AssetUtils.getKeyring(coinType, chainId);
    }

    public interface Callback {
        void onKeyringInfoReady(KeyringInfo keyringInfo);
    }

    @Override
    public void keyringCreated(@KeyringId.EnumType int keyringId) {
        update();
    }

    @Override
    public void keyringRestored(@KeyringId.EnumType int keyringId) {
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
    public void accountsAdded(AccountInfo[] addedAccounts) {}

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
