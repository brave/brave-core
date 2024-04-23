/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import static android.hardware.biometrics.BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED;

import android.annotation.SuppressLint;
import android.content.Context;
import android.hardware.biometrics.BiometricPrompt;
import android.hardware.biometrics.BiometricPrompt.AuthenticationCallback;
import android.hardware.biometrics.BiometricPrompt.AuthenticationResult;
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
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.io.IOException;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableEntryException;
import java.security.cert.CertificateException;
import java.util.concurrent.Executor;

import javax.crypto.BadPaddingException;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;

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

        final OnNextPage onNextPage = mOnNextPage;
        if (onNextPage == null) {
            // mOnNextPage might be {@code null} when detached from the screen.
            // It's very unlikely to happen during on view creation but better be extra
            // safe and return immediately.
            return;
        }
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

                    final KeyringService keyringService = getKeyringService();
                    if (keyringService != null) {
                        keyringService.unlock(
                                mUnlockWalletPassword.getText().toString(),
                                result -> {
                                    if (result) {
                                        Utils.clearClipboard(
                                                mUnlockWalletPassword.getText().toString());
                                        mUnlockWalletPassword.setText(null);
                                        onNextPage.onboardingCompleted();
                                    } else {
                                        mUnlockWalletPassword.setError(
                                                getString(R.string.incorrect_password_error));
                                    }
                                });
                    }
                });

        mUnlockWalletRestoreButton.setOnClickListener(
                v -> {
                    onNextPage.gotoRestorePage(false);
                    mUnlockWalletPassword.getText().clear();
                });

        mBiometricUnlockWalletImage.setOnClickListener(
                v -> {
                    if (Utils.isBiometricSupported(requireContext())) {
                        // noinspection NewApi
                        createBiometricPrompt();
                    }
                });

        if (KeystoreHelper.shouldUseBiometricToUnlock()
                && Utils.isBiometricSupported(requireContext())) {

            // noinspection NewApi
            createBiometricPrompt();
        } else {
            showPasswordRelatedControls();
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
        final AuthenticationCallback authenticationCallback =
                new AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);

                        final KeyringService keyringService = getKeyringService();
                        String unlockWalletPassword = null;

                        try {
                            unlockWalletPassword = KeystoreHelper.decryptText();
                        } catch (InvalidAlgorithmParameterException
                                | UnrecoverableEntryException
                                | NoSuchPaddingException
                                | IllegalBlockSizeException
                                | CertificateException
                                | KeyStoreException
                                | NoSuchAlgorithmException
                                | BadPaddingException
                                | IOException
                                | InvalidKeyException e) {
                            // KeystoreHelper.decryptText() may throw a long list
                            // of exceptions.
                            showPasswordRelatedControls();
                        }

                        if (TextUtils.isEmpty(unlockWalletPassword) || keyringService == null) {
                            showPasswordRelatedControls();
                            return;
                        }

                        keyringService.unlock(
                                unlockWalletPassword,
                                unlockResult -> {
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
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        final Context context = getContext();
                        if (!TextUtils.isEmpty(errString) && context != null) {
                            Toast.makeText(context, errString, Toast.LENGTH_SHORT).show();
                        }
                        showPasswordRelatedControls();
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showFingerprintDialog(
            @NonNull final AuthenticationCallback authenticationCallback) {
        Executor executor = ContextCompat.getMainExecutor(requireContext());
        new BiometricPrompt.Builder(requireContext())
                .setTitle(getResources().getString(R.string.fingerprint_unlock))
                .setDescription(getResources().getString(R.string.use_fingerprint_text))
                .setNegativeButton(
                        getResources().getString(android.R.string.cancel),
                        executor,
                        (dialog, which) ->
                                authenticationCallback.onAuthenticationError(
                                        BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    private void showPasswordRelatedControls() {
        mUnlockWalletPassword.setVisibility(View.VISIBLE);
        mUnlockButton.setVisibility(View.VISIBLE);
        mUnlockWalletRestoreButton.setVisibility(View.VISIBLE);
        mUnlockWalletTitle.setVisibility(View.VISIBLE);
        if (Utils.isBiometricSupported(requireContext())
                && KeystoreHelper.shouldUseBiometricToUnlock()) {
            mBiometricUnlockWalletImage.setVisibility(View.VISIBLE);
        }
    }
}
