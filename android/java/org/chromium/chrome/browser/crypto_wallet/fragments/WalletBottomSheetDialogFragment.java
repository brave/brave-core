/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.content.Context;
import android.content.DialogInterface;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.build.annotations.EnsuresNonNull;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;

/**
 * Base class for {@code BottomSheetDialogFragment} with wallet specific implementation
 * (auto-dismiss when locked, clean up etc).
 */
@NullMarked
public class WalletBottomSheetDialogFragment extends BottomSheetDialogFragment
        implements KeyringServiceObserverImpl.KeyringServiceObserverImplDelegate {

    private final KeyringServiceObserverImpl mKeyringObserver;

    @MonotonicNonNull private WalletModel mWalletModel;
    @MonotonicNonNull private KeyringModel mKeyringModel;

    public WalletBottomSheetDialogFragment() {
        mKeyringObserver = new KeyringServiceObserverImpl(this);
    }

    @EnsuresNonNull("mKeyringModel")
    protected KeyringModel getKeyringModel() {
        assert mKeyringModel != null;
        return mKeyringModel;
    }

    @EnsuresNonNull("mWalletModel")
    protected WalletModel getWalletModel() {
        assert mWalletModel != null;
        return mWalletModel;
    }

    protected BraveWalletService getBraveWalletService() {
        assert mWalletModel != null;
        return mWalletModel.getBraveWalletService();
    }

    protected KeyringService getKeyringService() {
        assert mWalletModel != null;
        return mWalletModel.getKeyringService();
    }

    protected JsonRpcService getJsonRpcService() {
        assert mWalletModel != null;
        return mWalletModel.getJsonRpcService();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);

        if (context instanceof WalletFragmentCallback walletFragmentCallback) {
            mWalletModel = walletFragmentCallback.getWalletModel();
            mKeyringModel = mWalletModel.getKeyringModel();
            mKeyringModel.registerKeyringObserver(mKeyringObserver);
        } else {
            throw new IllegalStateException("Host activity must implement WalletFragmentCallback.");
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        mKeyringObserver.close();
        super.onDismiss(dialog);
    }

    @Override
    public void locked() {
        dismiss();
    }
}
