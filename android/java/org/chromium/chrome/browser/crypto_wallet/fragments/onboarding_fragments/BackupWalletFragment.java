/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;

public class BackupWalletFragment extends CryptoOnboardingFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_backup_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Button backupWalletButton = view.findViewById(R.id.btn_backup_wallet_continue);
        backupWalletButton.setOnClickListener(v -> onNextPage.gotoNextPage(false));
        CheckBox backupWalletCheckbox = view.findViewById(R.id.backup_wallet_checkbox);
        backupWalletCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                backupWalletButton.setEnabled(true);
                backupWalletButton.setAlpha(1.0f);
            } else {
                backupWalletButton.setEnabled(false);
                backupWalletButton.setAlpha(0.5f);
            }
        });
        TextView backupWalletSkipButton = view.findViewById(R.id.btn_backup_wallet_skip);
        backupWalletSkipButton.setOnClickListener(v -> onNextPage.gotoNextPage(true));
    }
}
