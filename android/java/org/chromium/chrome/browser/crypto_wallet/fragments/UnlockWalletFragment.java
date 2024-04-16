/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
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

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentActivity;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

public class UnlockWalletFragment extends BaseWalletNextPageFragment {
    private EditText mUnlockWalletPassword;
    private Button mUnlockButton;
    private TextView mUnlockWalletRestoreButton;
    private TextView mUnlockWalletTitle;
    private ImageView mBiometricUnlockWalletImage;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
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

        mUnlockButton.setOnClickListener(
                v -> {
                    if (TextUtils.isEmpty(mUnlockWalletPassword.getText())) {
                        mUnlockWalletPassword.setError(getString(R.string.password_error));
                        return;
                    }

                    KeyringService keyringService = getKeyringService();
                    if (keyringService != null) {
                        keyringService.unlock(
                                mUnlockWalletPassword.getText().toString(),
                                result -> {
                                    if (result) {
                                        Utils.clearClipboard(
                                                mUnlockWalletPassword.getText().toString(), 0);
                                        mUnlockWalletPassword.setText(null);
                                        if (mOnNextPage != null) {
                                            Utils.hideKeyboard(requireActivity());
                                            mOnNextPage.onboardingCompleted();
                                        }
                                    } else {
                                        mUnlockWalletPassword.setError(
                                                getString(R.string.incorrect_password_error));
                                    }
                                });
                    }
                });

        mUnlockWalletRestoreButton.setOnClickListener(
                v -> {
                    if (mOnNextPage != null) {
                        mOnNextPage.gotoRestorePage(false);
                        mUnlockWalletPassword.getText().clear();
                    }
                });

        mBiometricUnlockWalletImage.setOnClickListener(
                v -> {
                    if (Utils.isBiometricAvailable(requireContext())) {
                        // noinspection NewApi
                        createBiometricPrompt();
                    }
                });

        if (mOnNextPage != null) {
            if (mOnNextPage.showBiometricPrompt()) {
                if (KeystoreHelper.shouldUseBiometricOnUnlock()
                        && Utils.isBiometricAvailable(requireContext())) {
                    // noinspection NewApi
                    createBiometricPrompt();
                } else {
                    showPasswordRelatedControls();
                }
            } else {
                mOnNextPage.enableBiometricPrompt();
                showPasswordRelatedControls();
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mOnNextPage != null) {
            mOnNextPage.showCloseButton(false);
            mOnNextPage.showBackButton(false);
        }

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
                                    if (mOnNextPage != null) {
                                        mOnNextPage.onboardingCompleted();
                                    }
                                } else {
                                    showPasswordRelatedControls();
                                    mUnlockWalletPassword.setError(
                                            getString(R.string.incorrect_password_error));
                                }
                            });
                        } catch (Exception exc) {
                            showPasswordRelatedControls();
                        }
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(requireActivity(), errString, Toast.LENGTH_SHORT).show();
                        }
                        // Even though we have an error, we still let to proceed
                        showPasswordRelatedControls();
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showFingerprintDialog(
            @NonNull final BiometricPrompt.AuthenticationCallback authenticationCallback) {
        Executor executor = ContextCompat.getMainExecutor(requireActivity());
        new BiometricPrompt.Builder(requireActivity())
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
        if (Utils.isBiometricAvailable(requireContext())
                && KeystoreHelper.shouldUseBiometricOnUnlock()) {
            mBiometricUnlockWalletImage.setVisibility(View.VISIBLE);
        }
    }
}
