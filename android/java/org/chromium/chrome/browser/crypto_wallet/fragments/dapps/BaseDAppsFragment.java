/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.app.Activity;
import android.content.Context;

import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletFragmentCallback;

public class BaseDAppsFragment extends Fragment {
    @Override
    public void onAttach(Context context) {
        super.onAttach(context);

        if (!(context instanceof WalletFragmentCallback)) {
            throw new IllegalStateException("Host activity must implement WalletFragmentCallback.");
        }
    }

    /** Returns the panel hosting this fragment, which {@link #onAttach} has checked. */
    protected WalletFragmentCallback getWalletFragmentCallback() {
        return (WalletFragmentCallback) requireActivity();
    }

    public BraveWalletService getBraveWalletService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getBraveWalletService();
        }

        return null;
    }

    public KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getKeyringService();
        }

        return null;
    }

    public JsonRpcService getJsonRpcService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getJsonRpcService();
        }

        return null;
    }
}
