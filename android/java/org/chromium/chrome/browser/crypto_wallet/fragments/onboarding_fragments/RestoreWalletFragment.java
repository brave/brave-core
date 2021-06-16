/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.os.Bundle;
import android.text.TextUtils;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class RestoreWalletFragment extends CryptoOnboardingFragment {
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_restore_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        EditText recoveryPhraseText = view.findViewById(R.id.recovery_phrase_text);

        ImageView restoreWalletCopyImage = view.findViewById(R.id.restore_wallet_copy_image);
        assert getActivity() != null;
        restoreWalletCopyImage.setOnClickListener(
                v -> recoveryPhraseText.setText(Utils.getTextFromClipboard(getActivity())));

        CheckBox showRecoveryPhraseCheckbox = view.findViewById(R.id.restore_wallet_checkbox);
        showRecoveryPhraseCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                recoveryPhraseText.setTransformationMethod(
                        HideReturnsTransformationMethod.getInstance());
            } else {
                recoveryPhraseText.setTransformationMethod(
                        PasswordTransformationMethod.getInstance());
            }
        });

        EditText passwordEdittext = view.findViewById(R.id.restore_wallet_password);
        EditText retypePasswordEdittext = view.findViewById(R.id.restore_wallet_retype_password);

        Button secureCryptoButton = view.findViewById(R.id.btn_restore_wallet);
        secureCryptoButton.setOnClickListener(v -> {
            String passwordInput = passwordEdittext.getText().toString().trim();
            String retypePasswordInput = retypePasswordEdittext.getText().toString().trim();

            if (passwordInput.isEmpty()
                    || !Utils.PASSWORD_PATTERN.matcher(passwordInput).matches()) {
                passwordEdittext.setError(getResources().getString(R.string.password_text));
            } else if (retypePasswordInput.isEmpty()
                    || !passwordInput.equals(retypePasswordInput)) {
                retypePasswordEdittext.setError(
                        getResources().getString(R.string.retype_password_error));
            } else {
                // Restore button
            }
        });
    }
}