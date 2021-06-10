/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import org.chromium.chrome.R;

import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;

import java.util.concurrent.Executor;

public class SecurePasswordFragment extends CryptoOnboardingFragment {
    private EditText passwordEdittext;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_secure_password, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        passwordEdittext = view.findViewById(R.id.secure_crypto_password);
        EditText retypePasswordEdittext = view.findViewById(R.id.secure_crypto_retype_password);

        Button secureCryptoButton = view.findViewById(R.id.btn_secure_crypto_continue);
        secureCryptoButton.setOnClickListener(v -> {
            String passwordInput = passwordEdittext.getText().toString().trim();
            String retypePasswordInput = retypePasswordEdittext.getText().toString().trim();

            if (passwordInput.isEmpty() || !Utils.PASSWORD_PATTERN.matcher(passwordInput).matches()) {
                passwordEdittext.setError(getResources().getString(R.string.password_text));
            } else if (retypePasswordInput.isEmpty() || !passwordInput.equals(retypePasswordInput)) {
                retypePasswordEdittext.setError(getResources().getString(R.string.retype_password_error));
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    showFingerprintDialog(authenticationCallback);
                } else {
                    Utils.recoveryPhrase = BraveWalletNativeWorker.getInstance().createWallet(passwordInput).trim();
                    onNextPage.gotoNextPage(false);
                }
            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showFingerprintDialog(
            @NonNull final BiometricPrompt.AuthenticationCallback authenticationCallback) {
        assert getActivity() != null;
        Executor executor = ContextCompat.getMainExecutor(getActivity());
        new BiometricPrompt.Builder(getActivity())
                .setTitle(getResources().getString(R.string.enable_fingerprint_unlock))
                .setDescription(getResources().getString(R.string.enable_fingerprint_text))
                .setNegativeButton(getResources().getString(android.R.string.cancel), executor,
                        (dialog, which)
                                -> authenticationCallback.onAuthenticationError(
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED,
                                        "User canceled the scanning process by pressing the negative button"))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    BiometricPrompt.AuthenticationCallback
            authenticationCallback = new BiometricPrompt.AuthenticationCallback() {
        @Override
        public void onAuthenticationSucceeded(BiometricPrompt.AuthenticationResult result) {
            super.onAuthenticationSucceeded(result);
            // Go to next Page
            Utils.recoveryPhrase = BraveWalletNativeWorker.getInstance().createWallet(passwordEdittext.getText().toString().trim()).trim();
            onNextPage.gotoNextPage(false);
        }

        @Override
        public void onAuthenticationError(int errorCode, CharSequence errString) {
            super.onAuthenticationError(errorCode, errString);
            switch (errorCode) {
                case BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED:
                    Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                    break;
                case BiometricPrompt.BIOMETRIC_ERROR_HW_NOT_PRESENT:
                case BiometricPrompt.BIOMETRIC_ERROR_HW_UNAVAILABLE:
                    Toast.makeText(getActivity(),
                                 "Device doesn't have the supported fingerprint hardware.",
                                 Toast.LENGTH_SHORT)
                            .show();
                    break;
                case BiometricPrompt.BIOMETRIC_ERROR_NO_BIOMETRICS:
                    Toast.makeText(getActivity(), "User did not register any fingerprints.",
                                 Toast.LENGTH_SHORT)
                            .show();
                    break;
                default:
                    Toast.makeText(getActivity(), "unrecoverable error", Toast.LENGTH_SHORT).show();
            }
        }
    };
}
