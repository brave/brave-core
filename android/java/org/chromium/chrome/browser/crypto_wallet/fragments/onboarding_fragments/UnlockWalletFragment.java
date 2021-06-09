/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;

public class UnlockWalletFragment extends CryptoOnboardingFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_unlock_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        EditText unlockWalletPassword = view.findViewById(R.id.unlock_wallet_password);

        Button unlockButton = view.findViewById(R.id.btn_unlock);
        unlockButton.setOnClickListener(v -> {
            if (TextUtils.isEmpty(unlockWalletPassword.getText())) {
                unlockWalletPassword.setError(getString(R.string.password_error));
            }
            onNextPage.gotoNextPage(true);
        });

        TextView unlockWalletRestoreButton = view.findViewById(R.id.btn_unlock_wallet_restore);
        unlockWalletRestoreButton.setOnClickListener(v -> onNextPage.gotoRestorePage());
    }
}
