/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.app.Activity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
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

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

public class RestoreWalletFragment extends CryptoOnboardingFragment {
    private EditText recoveryPhraseText;
    private EditText passwordEdittext;
    private EditText retypePasswordEdittext;
    private CheckBox showRecoveryPhraseCheckbox;
    private CheckBox restoreLegacyWalletCheckbox;
    private boolean isLegacyWalletRestoreEnable;

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_restore_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        recoveryPhraseText = view.findViewById(R.id.recovery_phrase_text);
        passwordEdittext = view.findViewById(R.id.restore_wallet_password);
        retypePasswordEdittext = view.findViewById(R.id.restore_wallet_retype_password);
        showRecoveryPhraseCheckbox = view.findViewById(R.id.restore_wallet_checkbox);
        restoreLegacyWalletCheckbox = view.findViewById(R.id.restore_legacy_wallet_checkbox);

        ImageView restoreWalletCopyImage = view.findViewById(R.id.restore_wallet_copy_image);
        assert getActivity() != null;
        restoreWalletCopyImage.setOnClickListener(
                v -> recoveryPhraseText.setText(Utils.getTextFromClipboard(getActivity())));

        showRecoveryPhraseCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            int cursorPos = recoveryPhraseText.getSelectionStart();
            if (isChecked) {
                recoveryPhraseText.setTransformationMethod(
                        HideReturnsTransformationMethod.getInstance());
            } else {
                recoveryPhraseText.setTransformationMethod(
                        PasswordTransformationMethod.getInstance());
            }
            recoveryPhraseText.setSelection(cursorPos);
        });

        recoveryPhraseText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void afterTextChanged(Editable editable) {}

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                String recoveryPhrase = charSequence.toString().trim();

                // validate recoveryPhrase contains only string. not JSON and length is 24
                if (recoveryPhrase.matches("[a-zA-Z\\s]+")
                        && recoveryPhrase.split("\\s+").length == 24) {
                    restoreLegacyWalletCheckbox.setVisibility(View.VISIBLE);
                } else {
                    restoreLegacyWalletCheckbox.setVisibility(View.GONE);
                }
            }
        });

        restoreLegacyWalletCheckbox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> { isLegacyWalletRestoreEnable = isChecked; });

        Button secureCryptoButton = view.findViewById(R.id.btn_restore_wallet);
        secureCryptoButton.setOnClickListener(v -> {
            String passwordInput = passwordEdittext.getText().toString();

            KeyringService keyringService = getKeyringService();
            assert keyringService != null;
            keyringService.isStrongPassword(passwordInput, result -> {
                if (!result) {
                    passwordEdittext.setError(getResources().getString(R.string.password_text));

                    return;
                }

                proceedWithAStrongPassword(passwordInput, view, recoveryPhraseText);
            });
        });
    }

    private void proceedWithAStrongPassword(
            String passwordInput, View view, EditText recoveryPhraseText) {
        String retypePasswordInput = retypePasswordEdittext.getText().toString();

        if (!passwordInput.equals(retypePasswordInput)) {
            retypePasswordEdittext.setError(
                    getResources().getString(R.string.retype_password_error));
        } else {
            KeyringService keyringService = getKeyringService();
            assert keyringService != null;
            keyringService.restoreWallet(recoveryPhraseText.getText().toString().trim(),
                    passwordInput, isLegacyWalletRestoreEnable, result -> {
                        if (result) {
                            Utils.hideKeyboard(getActivity());
                            onNextPage.gotoNextPage(true);
                            Utils.setCryptoOnboarding(false);
                            keyringService.notifyWalletBackupComplete();
                            Utils.clearClipboard(recoveryPhraseText.getText().toString().trim(), 0);
                            Utils.clearClipboard(passwordInput, 0);
                            Utils.clearClipboard(retypePasswordInput, 0);
                            cleanUp();
                        } else {
                            Toast.makeText(getActivity(), R.string.account_recovery_failed,
                                         Toast.LENGTH_SHORT)
                                    .show();
                        }
                    });
        }
    }

    private void cleanUp() {
        recoveryPhraseText.getText().clear();
        passwordEdittext.getText().clear();
        retypePasswordEdittext.getText().clear();
        showRecoveryPhraseCheckbox.setChecked(false);
        restoreLegacyWalletCheckbox.setChecked(false);
    }
}