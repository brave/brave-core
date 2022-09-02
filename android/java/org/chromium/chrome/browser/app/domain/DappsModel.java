/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.Pair;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class DappsModel {
    private JsonRpcService mJsonRpcService;
    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
    private PendingTxHelper mPendingTxHelper;
    private final MutableLiveData<Boolean> _mWalletIconNotificationVisible =
            new MutableLiveData<>(false);
    public final LiveData<Boolean> mWalletIconNotificationVisible = _mWalletIconNotificationVisible;
    private final Object mLock = new Object();

    public DappsModel(JsonRpcService jsonRpcService, BraveWalletService braveWalletService,
            KeyringService keyringService, PendingTxHelper pendingTxHelper) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mKeyringService = keyringService;
        mPendingTxHelper = pendingTxHelper;
    }

    public void fetchAccountsForConnectionReq(@CoinType.EnumType int coinType,
            Callbacks.Callback1<Pair<String, List<AccountInfo>>> callback) {
        if (coinType == CoinType.ETH || coinType == CoinType.SOL) {
            mKeyringService.getKeyringInfo(Utils.getKeyringForCoinType(coinType), keyringInfo -> {
                mKeyringService.getSelectedAccount(coinType, accountAddress -> {
                    if (coinType == CoinType.SOL) {
                        // only the selected account is used for solana dapps
                        for (AccountInfo accountInfo : keyringInfo.accountInfos) {
                            if (accountAddress.equals(accountInfo.address)) {
                                List<AccountInfo> accountInfos = new ArrayList<>();
                                accountInfos.add(accountInfo);
                                callback.call(new Pair<>(accountAddress, accountInfos));
                                return;
                            }
                        }
                    } else {
                        callback.call(new Pair<>(
                                accountAddress, Arrays.asList(keyringInfo.accountInfos)));
                    }
                });
            });
        } else {
            callback.call(new Pair<>(null, Collections.emptyList()));
        }
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

    public void setWalletBadgeVisible() {
        _mWalletIconNotificationVisible.setValue(true);
    }

    public void setWalletBadgeInvisible() {
        _mWalletIconNotificationVisible.setValue(false);
    }

    private void updateWalletBadgeVisibilityInternal() {
        if (mBraveWalletService == null || mJsonRpcService == null || mPendingTxHelper == null) {
            return;
        }

        _mWalletIconNotificationVisible.setValue(false);

        mBraveWalletService.getPendingSignMessageRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        mBraveWalletService.getPendingAddSuggestTokenRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        mJsonRpcService.getPendingAddChainRequests(networks -> {
            if (networks != null && networks.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        mJsonRpcService.getPendingSwitchChainRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        mBraveWalletService.getPendingDecryptRequests(requests -> {
            if (requests != null && requests.length > 0) {
                setWalletBadgeVisible();
                return;
            }
        });
        for (TransactionInfo info : mPendingTxHelper.mTransactionInfoLd.getValue()) {
            if (info.txStatus == TransactionStatus.UNAPPROVED) {
                setWalletBadgeVisible();
                break;
            }
        }
    }
}
