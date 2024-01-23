/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;

/**
 * Base Brave Wallet fragment that performs a cast on the host activity to extract {@link
 * OnNextPage} interface used for basic navigation actions.
 */
public abstract class BaseWalletNextPageFragment extends Fragment {

    // Might be {@code null} when detached from the screen.
    @Nullable protected OnNextPage mOnNextPage;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        try {
            mOnNextPage = (OnNextPage) context;
        } catch (ClassCastException e) {
            throw new ClassCastException("Host activity must implement OnNextPage interface.");
        }
    }

    @Override
    public void onDetach() {
        mOnNextPage = null;
        super.onDetach();
    }

    @Nullable
    protected KeyringService getKeyringService() {
        Activity activity = requireActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    @Nullable
    protected BraveWalletP3a getBraveWalletP3A() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletP3A();
        }

        return null;
    }
}
