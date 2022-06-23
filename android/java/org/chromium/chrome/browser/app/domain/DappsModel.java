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


public class DappsModel {
    private JsonRpcService mJsonRpcService;
    private BraveWalletService mBraveWalletService;
    private CryptoModel mCryptoModel;
    private PendingTxHelper mPendingTxHelper;
    private final MutableLiveData<Boolean> _mWalletIconNotificationVisible =
            new MutableLiveData<>(false);
    public final LiveData<Boolean> mWalletIconNotificationVisible = _mWalletIconNotificationVisible;

    public DappsModel(JsonRpcService jsonRpcService,
            BraveWalletService braveWalletService, PendingTxHelper pendingTxHelper) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mPendingTxHelper = pendingTxHelper;
    }

    public void resetServices(JsonRpcService jsonRpcService,
            BraveWalletService braveWalletService, PendingTxHelper pendingTxHelper) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mPendingTxHelper = pendingTxHelper;
    }

    public void updateWalletBadgeVisibility() {
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

    public void setWalletBadgeVisible() {
        _mWalletIconNotificationVisible.setValue(true);
    }

    public void setWalletBadgeInvisible() {
        _mWalletIconNotificationVisible.setValue(false);
    }
}
