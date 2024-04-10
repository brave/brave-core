/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModel;

import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.HashSet;
import java.util.Set;

public class OnboardingViewModel extends ViewModel {
    @Nullable private String mPassword;

    @NonNull final Set<NetworkInfo> mSelectedNetworks = new HashSet<>();
    @NonNull final Set<NetworkInfo> mAvailableNetworks = new HashSet<>();

    public void setPassword(@NonNull final String password) {
        mPassword = password;
    }

    @NonNull
    public String getPassword() {
        if (mPassword == null) {
            throw new IllegalStateException("Wallet password must not be null.");
        }
        return mPassword;
    }

    public void setSelectedNetworks(
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Set<NetworkInfo> availableNetworks) {
        mSelectedNetworks.clear();
        mSelectedNetworks.addAll(selectedNetworks);

        mAvailableNetworks.clear();
        mAvailableNetworks.addAll(availableNetworks);
    }

    @NonNull
    public Set<NetworkInfo> getSelectedNetworks() {
        return mSelectedNetworks;
    }

    @NonNull
    public Set<NetworkInfo> getAvailableNetworks() {
        return mAvailableNetworks;
    }
}
