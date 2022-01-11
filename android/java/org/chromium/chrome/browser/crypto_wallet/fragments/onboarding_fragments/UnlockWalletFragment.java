/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.app.Activity;
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
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.concurrent.Executor;

public class UnlockWalletFragment extends CryptoOnboardingFragment {
    private EditText mUnlockWalletPassword;
    private Button mUnlockButton;
    private TextView mUnlockWalletRestoreButton;
    private TextView mUnlockWalletTitle;
    private ImageView mBiometricUnlockWalletImage;

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
        return inflater.inflate(R.layout.fragment_unlock_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mUnlockWalletPassword = view.findViewById(R.id.unlock_wallet_password);
        mUnlockButton = view.findViewById(R.id.btn_unlock);
        mUnlockWalletRestoreButton = view.findViewById(R.id.btn_unlock_wallet_restore);
        mUnlockWalletTitle = view.findViewById(R.id.unlock_wallet_title);
        mBiometricUnlockWalletImage = view.findViewById(R.id.iv_biometric_unlock_wallet);

        mUnlockButton.setOnClickListener(v -> {
            if (TextUtils.isEmpty(mUnlockWalletPassword.getText())) {
                mUnlockWalletPassword.setError(getString(R.string.password_error));
                return;
            }

            KeyringService keyringService = getKeyringService();
            if (keyringService != null) {
                keyringService.unlock(mUnlockWalletPassword.getText().toString(), result -> {
                    if (result) {
                        Utils.clearClipboard(mUnlockWalletPassword.getText().toString(), 0);
                        mUnlockWalletPassword.setText(null);
                        if (onNextPage != null) {
                            Utils.hideKeyboard(getActivity());
                            onNextPage.gotoNextPage(true);
                        }
                    } else {
                        mUnlockWalletPassword.setError(
                                getString(R.string.incorrect_password_error));
                    }
                });
            }
        });

        mUnlockWalletRestoreButton.setOnClickListener(v -> {
            if (onNextPage != null) {
                onNextPage.gotoRestorePage();
                mUnlockWalletPassword.getText().clear();
            }
        });

        mBiometricUnlockWalletImage.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                createBiometricPrompt();
            }
        });

        if (onNextPage != null && onNextPage.showBiometricPrompt()) {
            checkOnBiometric();
        } else if (onNextPage != null) {
            onNextPage.showBiometricPrompt(true);
            showPasswordRelatedControls();
        }
    }

    private void checkOnBiometric() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P
                || !KeystoreHelper.shouldUseBiometricOnUnlock()) {
            showPasswordRelatedControls();

            return;
        }

        createBiometricPrompt();
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void createBiometricPrompt() {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        // We authenticated using fingerprint
                        try {
                            String unlockWalletPassword = KeystoreHelper.decryptText();
                            if (unlockWalletPassword.isEmpty()) {
                                showPasswordRelatedControls();

                                return;
                            }
                            KeyringService keyringService = getKeyringService();
                            assert keyringService != null;
                            keyringService.unlock(unlockWalletPassword, unlockResult -> {
                                if (unlockResult) {
                                    if (onNextPage != null) {
                                        onNextPage.gotoNextPage(true);
                                    }
                                } else {
                                    showPasswordRelatedControls();
                                    mUnlockWalletPassword.setError(
                                            getString(R.string.incorrect_password_error));
                                }
                            });
                        } catch (Exception exc) {
                            showPasswordRelatedControls();

                            return;
                        }
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                        }
                        // Even though we have an error, we still let to proceed
                        showPasswordRelatedControls();
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showFingerprintDialog(
            @NonNull final BiometricPrompt.AuthenticationCallback authenticationCallback) {
        assert getActivity() != null;
        Executor executor = ContextCompat.getMainExecutor(getActivity());
        new BiometricPrompt.Builder(getActivity())
                .setTitle(getResources().getString(R.string.fingerprint_unlock))
                .setDescription(getResources().getString(R.string.use_fingerprint_text))
                .setNegativeButton(getResources().getString(android.R.string.cancel), executor,
                        (dialog, which)
                                -> authenticationCallback.onAuthenticationError(
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    private void showPasswordRelatedControls() {
        mUnlockWalletPassword.setVisibility(View.VISIBLE);
        mUnlockButton.setVisibility(View.VISIBLE);
        mUnlockWalletRestoreButton.setVisibility(View.VISIBLE);
        mUnlockWalletTitle.setVisibility(View.VISIBLE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                && KeystoreHelper.shouldUseBiometricOnUnlock()) {
            mBiometricUnlockWalletImage.setVisibility(View.VISIBLE);
        }
    }
}
