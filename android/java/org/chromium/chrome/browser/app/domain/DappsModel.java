/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ByteArrayStringUnion;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringId;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.brave_wallet.mojom.SignAllTransactionsRequest;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.brave_wallet.mojom.SignTransactionRequest;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.model.WalletAccountCreationRequest;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class DappsModel implements KeyringServiceObserver {
    private JsonRpcService mJsonRpcService;
    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    private PendingTxHelper mPendingTxHelper;
    private final MutableLiveData<Boolean> _mWalletIconNotificationVisible =
            new MutableLiveData<>(false);
    private final Object mLock = new Object();
    private final MutableLiveData<BraveWalletDAppsActivity.ActivityType> _mProcessNextDAppsRequest =
            new MutableLiveData<>();
    private final MutableLiveData<List<SignTransactionRequest>> _mSignTxRequests;
    private final MutableLiveData<List<SignAllTransactionsRequest>> _mSignAllTxRequests;
    private final LiveData<List<SignTransactionRequest>> mSignTxRequests;
    private final LiveData<List<SignAllTransactionsRequest>> mSignAllTxRequests;
    private List<WalletAccountCreationRequest> mPendingWalletAccountCreationRequests;
    private MutableLiveData<WalletAccountCreationRequest> _mPendingWalletAccountCreationRequest;
    public LiveData<WalletAccountCreationRequest> mPendingWalletAccountCreationRequest;
    public final LiveData<Boolean> mWalletIconNotificationVisible = _mWalletIconNotificationVisible;
    public final LiveData<BraveWalletDAppsActivity.ActivityType> mProcessNextDAppsRequest =
            _mProcessNextDAppsRequest;

    public DappsModel(JsonRpcService jsonRpcService, BraveWalletService braveWalletService,
            KeyringService keyringService, PendingTxHelper pendingTxHelper) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mKeyringService = keyringService;
        mPendingTxHelper = pendingTxHelper;
        _mSignTxRequests = new MutableLiveData<>(Collections.emptyList());
        mSignTxRequests = _mSignTxRequests;
        _mSignAllTxRequests = new MutableLiveData<>(Collections.emptyList());
        mSignAllTxRequests = _mSignAllTxRequests;
        _mPendingWalletAccountCreationRequest = new MutableLiveData<>();
        mPendingWalletAccountCreationRequest = _mPendingWalletAccountCreationRequest;
        mPendingWalletAccountCreationRequests = new ArrayList<>();
        mKeyringService.addObserver(this);
    }

    public void fetchAccountsForConnectionReq(@CoinType.EnumType int coinType,
            Callbacks.Callback1<Pair<AccountInfo, List<AccountInfo>>> callback) {
        if (coinType != CoinType.ETH && coinType != CoinType.SOL) {
            callback.call(new Pair<>(null, Collections.emptyList()));
            return;
        }

        mKeyringService.getAllAccounts(allAccounts -> {
            if (coinType == CoinType.SOL) {
                // only the selected account is used for solana dapps
                if (allAccounts.solDappSelectedAccount != null) {
                    List<AccountInfo> accounts = new ArrayList();
                    accounts.add(allAccounts.solDappSelectedAccount);
                    callback.call(new Pair<>(allAccounts.solDappSelectedAccount, accounts));
                }
            } else {
                List<AccountInfo> accounts =
                        Utils.filterAccountsByCoin(allAccounts.accounts, coinType);
                callback.call(new Pair<>(allAccounts.ethDappSelectedAccount, accounts));
            }
        });
    }

    public LiveData<List<SignTransactionRequest>> fetchSignTxRequest() {
        mBraveWalletService.getPendingSignTransactionRequests(requests -> {
            _mSignTxRequests.postValue(new ArrayList<>(Arrays.asList(requests)));
        });
        return mSignTxRequests;
    }

    public void signTxRequest(boolean isApproved, SignTransactionRequest signTransactionRequest) {
        mBraveWalletService.notifySignTransactionRequestProcessed(
                isApproved, signTransactionRequest.id, null, null);
        mBraveWalletService.getPendingSignTransactionRequests(requests -> {
            if (requests.length == 0) {
                _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType.FINISH);
            } else {
                _mSignTxRequests.postValue(new ArrayList<>(Arrays.asList(requests)));
            }
        });
    }

    public LiveData<List<SignAllTransactionsRequest>> fetchSignAllTxRequest() {
        mBraveWalletService.getPendingSignAllTransactionsRequests(requests -> {
            _mSignAllTxRequests.postValue(new ArrayList<>(Arrays.asList(requests)));
        });
        return mSignAllTxRequests;
    }

    public void signAllTxRequest(boolean isApproved, SignAllTransactionsRequest request) {
        mBraveWalletService.notifySignAllTransactionsRequestProcessed(
                isApproved, request.id, null, null);
        mBraveWalletService.getPendingSignAllTransactionsRequests(requests -> {
            if (requests.length == 0) {
                _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType.FINISH);
            } else {
                _mSignAllTxRequests.postValue(new ArrayList<>(Arrays.asList(requests)));
            }
        });
    }

    public void resetServices(JsonRpcService jsonRpcService,
            BraveWalletService braveWalletService, PendingTxHelper pendingTxHelper) {
        synchronized (mLock) {
            mBraveWalletService = braveWalletService;
            mJsonRpcService = jsonRpcService;
            mPendingTxHelper = pendingTxHelper;
        }
    }

    public void updateWalletBadgeVisibility() {
        synchronized (mLock) {
            updateWalletBadgeVisibilityInternal();
        }
    }

    public void clearDappsState() {
        _mProcessNextDAppsRequest.postValue(null);
    }

    public void processPublicEncryptionKey(String requestId, boolean isApproved) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.notifyGetPublicKeyRequestProcessed(requestId, isApproved);
            mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
                if (requests != null && requests.length > 0) {
                    _mProcessNextDAppsRequest.postValue(BraveWalletDAppsActivity.ActivityType
                                                                .GET_ENCRYPTION_PUBLIC_KEY_REQUEST);
                } else {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.FINISH);
                }
            });
        }
    }

    public void processDecryptRequest(String requestId, boolean isApproved) {
        synchronized (mLock) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.notifyDecryptRequestProcessed(requestId, isApproved);
            mBraveWalletService.getPendingDecryptRequests(requests -> {
                if (requests != null && requests.length > 0) {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.DECRYPT_REQUEST);
                } else {
                    _mProcessNextDAppsRequest.postValue(
                            BraveWalletDAppsActivity.ActivityType.FINISH);
                }
            });
        }
    }

    public void setWalletBadgeVisible() {
        _mWalletIconNotificationVisible.setValue(true);
    }

    public void setWalletBadgeInvisible() {
        _mWalletIconNotificationVisible.setValue(false);
    }

    public void addAccountCreationRequest(@CoinType.EnumType int coinType) {
        Utils.removeIf(mPendingWalletAccountCreationRequests,
                request -> request.getCoinType() == coinType);
        WalletAccountCreationRequest request = new WalletAccountCreationRequest(coinType);
        mPendingWalletAccountCreationRequests.add(request);
        updatePendingAccountCreationRequest();
    }

    public void removeProcessedAccountCreationRequest(WalletAccountCreationRequest request) {
        if (request == null) return;
        Utils.removeIf(mPendingWalletAccountCreationRequests,
                input -> input.getCoinType() == request.getCoinType());
        updatePendingAccountCreationRequest();
    }

    public void showPendingAccountCreationRequest() {
        WalletAccountCreationRequest request = _mPendingWalletAccountCreationRequest.getValue();
        if (request != null) {
            _mPendingWalletAccountCreationRequest.postValue(request);
        } else if (!mPendingWalletAccountCreationRequests.isEmpty()) {
            _mPendingWalletAccountCreationRequest.postValue(
                    mPendingWalletAccountCreationRequests.get(0));
        }
    }

    public void notifySignMessageRequestProcessed(boolean isApproved, int id) {
        notifySignMessageRequestProcessed(isApproved, id, null, null);
    }
    public void notifySignMessageRequestProcessed(
            boolean isApproved, int id, ByteArrayStringUnion signature, String error) {
        mBraveWalletService.notifySignMessageRequestProcessed(isApproved, id, signature, error);
    }
    public void getPendingSignMessageRequests(Callbacks.Callback1<SignMessageRequest[]> callback) {
        mBraveWalletService.getPendingSignMessageRequests(callback::call);
    }

    private void updateWalletBadgeVisibilityInternal() {
        if (mBraveWalletService == null || mJsonRpcService == null || mPendingTxHelper == null) {
            return;
        }

        _mWalletIconNotificationVisible.setValue(false);

        mBraveWalletService.getPendingSignMessageRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mBraveWalletService.getPendingAddSuggestTokenRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mJsonRpcService.getPendingAddChainRequests(networks -> {
            if (networks != null && networks.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mJsonRpcService.getPendingSwitchChainRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mBraveWalletService.getPendingDecryptRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mBraveWalletService.getPendingSignTransactionRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        mBraveWalletService.getPendingSignAllTransactionsRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
            }
        });
        for (TransactionInfo info : mPendingTxHelper.mTransactionInfoLd.getValue()) {
            if (info.txStatus == TransactionStatus.UNAPPROVED) {
                setWalletBadgeVisible();
                break;
            }
        }
    }

    private void updatePendingAccountCreationRequest() {
        if (mPendingWalletAccountCreationRequests.isEmpty()) {
            _mPendingWalletAccountCreationRequest.postValue(null);
        } else {
            _mPendingWalletAccountCreationRequest.postValue(
                    mPendingWalletAccountCreationRequests.get(0));
        }
    }

    @Override
    public void keyringCreated(@KeyringId.EnumType int keyringId) {}

    @Override
    public void keyringRestored(@KeyringId.EnumType int keyringId) {}

    @Override
    public void keyringReset() {}

    @Override
    public void locked() {}

    @Override
    public void unlocked() {
        showPendingAccountCreationRequest();
    }

    @Override
    public void backedUp() {}

    @Override
    public void accountsChanged() {}

    @Override
    public void accountsAdded(AccountInfo[] addedAccounts) {}

    @Override
    public void autoLockMinutesChanged() {}

    @Override
    public void selectedWalletAccountChanged(AccountInfo accountInfo) {}

    @Override
    public void selectedDappAccountChanged(
            @CoinType.EnumType int coinType, AccountInfo accountInfo) {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}
}
