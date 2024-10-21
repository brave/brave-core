/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import static org.chromium.base.ThreadUtils.assertOnUiThread;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;

import org.chromium.base.Callbacks;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AllAccountsInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringId;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.mojo.system.MojoException;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class KeyringModel implements KeyringServiceObserver {
    private static final String TAG = "KeyringModel";
    private final Object mLock = new Object();

    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    private final MutableLiveData<AccountInfo> _mSelectedAccount;
    private final MutableLiveData<AllAccountsInfo> _mAllAccountsInfo;
    private final MutableLiveData<List<AccountInfo>> _mAccountInfos;
    private CryptoSharedActions mCryptoSharedActions;
    public LiveData<List<AccountInfo>> mAccountInfos;
    public LiveData<AccountInfo> mSelectedAccount;
    public LiveData<AllAccountsInfo> mAllAccountsInfo;

    public KeyringModel(
            KeyringService keyringService,
            BraveWalletService braveWalletService,
            CryptoSharedActions cryptoSharedActions) {
        mKeyringService = keyringService;
        mBraveWalletService = braveWalletService;
        mCryptoSharedActions = cryptoSharedActions;

        _mSelectedAccount = new MutableLiveData<>();
        mSelectedAccount = _mSelectedAccount;
        _mAllAccountsInfo = new MutableLiveData<>();
        mAllAccountsInfo = _mAllAccountsInfo;
        _mAccountInfos = new MutableLiveData<>(Collections.emptyList());
        mAccountInfos = _mAccountInfos;
    }

    public void init() {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.addObserver(this);
        }
    }

    public void update() {
        synchronized (mLock) {
            if (mKeyringService == null) {
                return;
            }
            mKeyringService.getAllAccounts(allAccounts -> {
                _mAllAccountsInfo.postValue(allAccounts);
                _mAccountInfos.postValue(Arrays.asList(allAccounts.accounts));
                _mSelectedAccount.postValue(allAccounts.selectedAccount);
            });
        }
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

    public void resetService(KeyringService keyringService, BraveWalletService braveWalletService) {
        synchronized (mLock) {
            mKeyringService = keyringService;
            mBraveWalletService = braveWalletService;
        }
        if (mKeyringService != null && mBraveWalletService != null) {
            init();
        }
    }

    public void getAccounts(Callbacks.Callback1<AccountInfo[]> callback1) {
        mKeyringService.getAllAccounts(allAccounts -> { callback1.call(allAccounts.accounts); });
    }

    public void isWalletCreated(@NonNull final KeyringService.IsWalletCreated_Response callback) {
        if (mKeyringService == null) {
            callback.call(false);
        } else {
            mKeyringService.isWalletCreated(callback);
        }
    }

    private void addAccountInternal(
            @CoinType.EnumType int coinType,
            @KeyringId.EnumType int keyringId,
            String accountName,
            Callbacks.Callback1<Boolean> callback) {
        mKeyringService.addAccount(
                coinType,
                keyringId,
                accountName,
                result -> {
                    handleAddAccountResult(result, callback);
                });
    }

    public void addAccount(
            @CoinType.EnumType int coinType,
            String chainId,
            String accountName,
            Callbacks.Callback1<Boolean> callback) {
        @KeyringId.EnumType int keyringId = AssetUtils.getKeyring(coinType, chainId);
        if (accountName == null) {
            LiveDataUtil.observeOnce(
                    mAccountInfos,
                    accounts -> {
                        addAccountInternal(
                                coinType,
                                keyringId,
                                WalletUtils.generateUniqueAccountName(
                                        coinType, accounts.toArray(new AccountInfo[0])),
                                callback);
                    });
        } else {
            addAccountInternal(coinType, keyringId, accountName, callback);
        }
    }

    public void isWalletLocked(@NonNull final Callbacks.Callback1<Boolean> callback) {
        mKeyringService.isLocked(callback::call);
    }

    public void registerKeyringObserver(KeyringServiceObserverImpl observer) {
        mKeyringService.addObserver(observer);
    }

    public AccountInfo getAccount(String address) {
        List<AccountInfo> accountInfoList = mAccountInfos.getValue();
        if (accountInfoList == null || accountInfoList.isEmpty()) return null;
        for (AccountInfo accountInfo : accountInfoList) {
            if (accountInfo.address.equals(address)) return accountInfo;
        }
        return null;
    }

    private void handleAddAccountResult(
            AccountInfo result, @NonNull final Callbacks.Callback1<Boolean> callback) {
        mCryptoSharedActions.updateCoinType();
        mCryptoSharedActions.onNewAccountAdded();
        callback.call(result != null);
    }

    /**
     * Restore a Brave Wallet with a given password, showing only the collection of selected
     * networks. Ethereum and Solana accounts will be restored using the recovery phrase; Bitcoin
     * account and Filecoin account will be created only if selected among available networks. Once
     * the restoration process finishes the callback is notified with a boolean.
     *
     * <p><b>Note:</b> This method must be always called from main UI thread.
     *
     * @param password Given password used to restore the Brave Wallet.
     * @param recoveryPhrase Recovery phrase used to restore the Brave Wallet.
     * @param legacyRestoreEnabled boolean flag to restore legacy Wallet.
     * @param availableNetworks All available networks.
     * @param selectedNetworks Collection of selected networks that will be shown.
     * @param jsonRpcService JSON RPC service used to add and hide the networks.
     * @param callback Callback fired once restoration terminates passing a boolean containing the
     *     result.
     */
    @MainThread
    public void restoreWallet(
            @NonNull final String password,
            @NonNull final String recoveryPhrase,
            final boolean legacyRestoreEnabled,
            @NonNull final Set<NetworkInfo> availableNetworks,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final JsonRpcService jsonRpcService,
            @NonNull final Callbacks.Callback1<Boolean> callback) {
        assertOnUiThread();
        generateWallet(
                password,
                recoveryPhrase,
                legacyRestoreEnabled,
                availableNetworks,
                selectedNetworks,
                jsonRpcService,
                null,
                callback);
    }

    /**
     * Creates a new Brave Wallet with a given password, showing only the collection of selected
     * networks. One Ethereum and one Solana account will be created by default; Bitcoin account and
     * Filecoin account will be created only if selected among available networks. Once the creation
     * finishes the callback is notified with a string containing the recovery phrases.
     *
     * <p><b>Note:</b> This method must be always called from main UI thread.
     *
     * @param password Given password used to create the new Brave Wallet.
     * @param availableNetworks All available networks.
     * @param selectedNetworks Collection of selected networks that will be shown.
     * @param jsonRpcService JSON RPC service used to add and hide the networks.
     * @param callback Callback fired once creation terminates passing a string containing the
     *     recovery phrases.
     */
    @MainThread
    public void createWallet(
            @NonNull final String password,
            @NonNull final Set<NetworkInfo> availableNetworks,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final JsonRpcService jsonRpcService,
            @NonNull final Callbacks.Callback1<String> callback) {
        assertOnUiThread();
        generateWallet(
                password,
                null,
                false,
                availableNetworks,
                selectedNetworks,
                jsonRpcService,
                callback,
                null);
    }

    private void generateWallet(
            @NonNull final String password,
            @Nullable final String recoveryPhrase,
            final boolean legacyRestoreEnabled,
            @NonNull final Set<NetworkInfo> availableNetworks,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final JsonRpcService jsonRpcService,
            @Nullable final Callbacks.Callback1<String> createCallback,
            @Nullable final Callbacks.Callback1<Boolean> restoreCallback) {
        final Set<NetworkInfo> removeHiddenNetworks = new HashSet<>();
        final Set<NetworkInfo> addHiddenNetworks = new HashSet<>();

        MutableLiveData<Boolean> removeHiddenNetworksLiveData = new MutableLiveData<>();
        MutableLiveData<Boolean> addHiddenNetworksLiveData = new MutableLiveData<>();

        for (NetworkInfo networkInfo : availableNetworks) {
            if (selectedNetworks.contains(networkInfo)) {
                removeHiddenNetworks.add(networkInfo);
            } else {
                addHiddenNetworks.add(networkInfo);
            }
        }

        // Subscribe observer only if are present hidden networks to remove.
        if (!removeHiddenNetworks.isEmpty()) {
            removeHiddenNetworksLiveData.observeForever(
                    new Observer<>() {
                        int countRemovedHiddenNetworks;

                        @Override
                        public void onChanged(Boolean success) {
                            countRemovedHiddenNetworks++;
                            if (countRemovedHiddenNetworks == removeHiddenNetworks.size()) {
                                removeHiddenNetworksLiveData.removeObserver(this);

                                if (!addHiddenNetworksLiveData.hasActiveObservers()) {
                                    if (recoveryPhrase != null && restoreCallback != null) {
                                        finalizeWalletRestoration(
                                                password,
                                                recoveryPhrase,
                                                legacyRestoreEnabled,
                                                selectedNetworks,
                                                restoreCallback);
                                    } else if (createCallback != null) {
                                        finalizeWalletCreation(
                                                password, selectedNetworks, createCallback);
                                    }
                                }
                            }
                        }
                    });
        }

        // Subscribe observer only if are present hidden networks to add.
        if (!addHiddenNetworks.isEmpty()) {
            addHiddenNetworksLiveData.observeForever(
                    new Observer<>() {
                        int countAddedHiddenNetworks;

                        @Override
                        public void onChanged(Boolean success) {
                            countAddedHiddenNetworks++;
                            if (countAddedHiddenNetworks == addHiddenNetworks.size()) {
                                addHiddenNetworksLiveData.removeObserver(this);

                                if (!removeHiddenNetworksLiveData.hasActiveObservers()) {
                                    if (recoveryPhrase != null && restoreCallback != null) {
                                        finalizeWalletRestoration(
                                                password,
                                                recoveryPhrase,
                                                legacyRestoreEnabled,
                                                selectedNetworks,
                                                restoreCallback);
                                    } else if (createCallback != null) {
                                        finalizeWalletCreation(
                                                password, selectedNetworks, createCallback);
                                    }
                                }
                            }
                        }
                    });
        }

        for (NetworkInfo networkInfo : removeHiddenNetworks) {
            jsonRpcService.removeHiddenNetwork(
                    networkInfo.coin,
                    networkInfo.chainId,
                    success -> {
                        if (!success) {
                            Log.w(
                                    TAG,
                                    String.format(
                                            Locale.ENGLISH,
                                            "Unable to remove network %s from hidden networks.",
                                            networkInfo.chainName));
                        }
                        removeHiddenNetworksLiveData.setValue(success);
                    });
        }

        for (NetworkInfo networkInfo : addHiddenNetworks) {
            jsonRpcService.addHiddenNetwork(
                    networkInfo.coin,
                    networkInfo.chainId,
                    success -> {
                        if (!success) {
                            Log.w(
                                    TAG,
                                    String.format(
                                            Locale.ENGLISH,
                                            "Unable to add network %s to hidden networks.",
                                            networkInfo.chainName));
                        }
                        addHiddenNetworksLiveData.setValue(success);
                    });
        }
    }

    private void finalizeWalletRestoration(
            @NonNull final String password,
            @NonNull final String recoveryPhrase,
            final boolean legacyRestoreEnabled,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Callbacks.Callback1<Boolean> callback) {
        assertOnUiThread();
        mKeyringService.restoreWallet(
                recoveryPhrase,
                password,
                legacyRestoreEnabled,
                result -> createAccounts(result, selectedNetworks, callback));
    }

    private void finalizeWalletCreation(
            @NonNull final String password,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Callbacks.Callback1<String> callback) {
        assertOnUiThread();
        mKeyringService.createWallet(
                password,
                recoveryPhrase -> createAccounts(recoveryPhrase, selectedNetworks, callback));
    }

    private <T> void createAccounts(
            final T result,
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Callbacks.Callback1<T> callback) {
        final Set<NetworkInfo> createAccounts = new HashSet<>();

        for (NetworkInfo networkInfo : selectedNetworks) {
            // Create Bitcoin and Filecoin accounts if they have been selected.
            if (networkInfo.coin == CoinType.BTC || networkInfo.coin == CoinType.FIL) {
                createAccounts.add(networkInfo);
            }
        }

        if (createAccounts.isEmpty()) {
            callback.call(result);
        } else {
            final MutableLiveData<Boolean> createAccountsLiveData = new MutableLiveData<>();

            createAccountsLiveData.observeForever(
                    new Observer<>() {
                        int countCreatedAccounts;

                        @Override
                        public void onChanged(Boolean success) {
                            countCreatedAccounts++;
                            if (countCreatedAccounts == createAccounts.size()) {
                                createAccountsLiveData.removeObserver(this);
                                // Set ETH account by default as initial state.
                                selectEthAccount(result, callback);
                            }
                        }
                    });

            LiveDataUtil.observeOnce(
                    mAccountInfos,
                    accounts -> {
                        for (NetworkInfo networkInfo : createAccounts) {
                            String accountName =
                                    WalletUtils.generateUniqueAccountName(
                                            networkInfo.coin, accounts.toArray(new AccountInfo[0]));
                            mKeyringService.addAccount(
                                    networkInfo.coin,
                                    AssetUtils.getKeyring(networkInfo.coin, networkInfo.chainId),
                                    accountName,
                                    accountInfo -> createAccountsLiveData.setValue(true));
                        }
                    });
        }
    }

    private <T> void selectEthAccount(
            final T result, @NonNull final Callbacks.Callback1<T> callback) {
        mKeyringService.getAllAccounts(
                allAccounts -> {
                    if (allAccounts.ethDappSelectedAccount != null) {
                        mKeyringService.setSelectedAccount(
                                allAccounts.ethDappSelectedAccount.accountId,
                                success -> callback.call(result));
                    } else {
                        callback.call(result);
                    }
                });
    }

    @Override
    public void walletCreated() {
        update();
    }

    @Override
    public void walletRestored() {
        update();
    }

    @Override
    public void walletReset() {
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
    public void selectedWalletAccountChanged(AccountInfo accountInfo) {
        update();
    }

    @Override
    public void selectedDappAccountChanged(
            @CoinType.EnumType int coinType, AccountInfo accountInfo) {
        update();
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}

    @StringDef({BraveWalletConstants.FILECOIN_MAINNET, BraveWalletConstants.FILECOIN_TESTNET})
    public @interface FilecoinNetworkType {}
}
