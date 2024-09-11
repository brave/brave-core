/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.content.DialogInterface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;

/**
 * Base class for {@code BottomSheetDialogFragment} with wallet specific implementation
 * (auto-dismiss when locked, clean up etc).
 */
public class WalletBottomSheetDialogFragment extends BottomSheetDialogFragment
        implements KeyringServiceObserverImpl.KeyringServiceObserverImplDelegate {
    private KeyringServiceObserverImpl mKeyringObserver;

    @Override
    public void onDismiss(@NonNull DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mKeyringObserver != null) {
            mKeyringObserver.close();
        }
    }

    protected void registerKeyringObserver(@Nullable final KeyringModel keyringModel) {
        if (keyringModel == null || mKeyringObserver != null) return;

        mKeyringObserver = new KeyringServiceObserverImpl(this);
        keyringModel.registerKeyringObserver(mKeyringObserver);
    }

    @Override
    public void locked() {
        dismiss();
    }
}
