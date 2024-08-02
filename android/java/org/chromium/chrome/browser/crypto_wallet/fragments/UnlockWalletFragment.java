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
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentActivity;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

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

    private TextInputEditText mUnlockWalletPassword;
    private TextInputLayout mUnlockWalletPasswordLayout;
    private Button mUnlockButton;
    private TextView mUnlockWalletRestoreButton;
    private ImageView mBiometricUnlockButton;

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
        setAnimatedBackground(view.findViewById(R.id.unlock_wallet_root));

        final OnNextPage onNextPage = mOnNextPage;
        if (onNextPage == null) {
            // mOnNextPage might be {@code null} when detached from the screen.
            // It's very unlikely to happen during on view creation but better be extra
            // safe and return immediately.
            return;
        }
        mUnlockWalletPassword = view.findViewById(R.id.unlock_wallet_password);
        mUnlockWalletPasswordLayout = view.findViewById(R.id.unlock_wallet_password_layout);
        mUnlockButton = view.findViewById(R.id.btn_unlock);
        mUnlockButton.setEnabled(false);
        mUnlockWalletRestoreButton = view.findViewById(R.id.btn_unlock_wallet_restore);
        mBiometricUnlockButton = view.findViewById(R.id.biometric_unlock_wallet);

        mUnlockWalletPassword.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(
                            CharSequence text, int start, int count, int after) {
                        /* Not used. */
                    }

                    @Override
                    public void onTextChanged(CharSequence text, int start, int before, int count) {
                        mUnlockButton.setEnabled(text.length() != 0);
                        mUnlockWalletPasswordLayout.setError(null);
                    }

                    @Override
                    public void afterTextChanged(Editable text) {
                        /* Not used. */
                    }
                });

        mUnlockButton.setOnClickListener(
                v -> {
                    final KeyringService keyringService = getKeyringService();
                    if (keyringService != null && mUnlockWalletPassword.getText() != null) {
                        keyringService.unlock(
                                mUnlockWalletPassword.getText().toString(),
                                result -> {
                                    if (result) {
                                        Utils.clearClipboard(
                                                mUnlockWalletPassword.getText().toString());
                                        mUnlockWalletPassword.setText(null);
                                        onNextPage.showWallet(false);
                                    } else {
                                        mUnlockWalletPasswordLayout.setError(
                                                getString(R.string.incorrect_password_error));
                                    }
                                });
                    }
                });

        mUnlockWalletRestoreButton.setOnClickListener(
                v -> {
                    onNextPage.gotoRestorePage(false);
                    mUnlockWalletPassword.setText(null);
                });

        mBiometricUnlockButton.setOnClickListener(
                v -> {
                    if (Utils.isBiometricSupported(requireContext())) {
                        // noinspection NewApi
                        showBiometricAuthenticationDialog();
                    }
                });

        if (KeystoreHelper.shouldUseBiometricToUnlock()
                && Utils.isBiometricSupported(requireContext())) {

            mBiometricUnlockButton.setVisibility(View.VISIBLE);
            // noinspection NewApi
            showBiometricAuthenticationDialog();
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

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showBiometricAuthenticationDialog() {
        final AuthenticationCallback authenticationCallback =
                new AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);

                        final KeyringService keyringService = getKeyringService();
                        String unlockWalletPassword;

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
                            showBiometricAuthenticationButton();
                            return;
                        }

                        if (TextUtils.isEmpty(unlockWalletPassword) || keyringService == null) {
                            showBiometricAuthenticationButton();
                            return;
                        }

                        keyringService.unlock(
                                unlockWalletPassword,
                                unlockResult -> {
                                    if (unlockResult) {
                                        if (mOnNextPage != null) {
                                            mOnNextPage.showWallet(false);
                                        }
                                    } else {
                                        showBiometricAuthenticationButton();
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
                        showBiometricAuthenticationButton();
                    }
                };
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

    /** Shows biometric authentication button if supported and if it was previously set. */
    private void showBiometricAuthenticationButton() {
        if (Utils.isBiometricSupported(requireContext())
                && KeystoreHelper.shouldUseBiometricToUnlock()) {
            mBiometricUnlockButton.setVisibility(View.VISIBLE);
        } else {
            mBiometricUnlockButton.setVisibility(View.GONE);
        }
    }
}
