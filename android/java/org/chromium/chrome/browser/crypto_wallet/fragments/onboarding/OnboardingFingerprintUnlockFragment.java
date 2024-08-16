/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.annotation.SuppressLint;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.widget.AppCompatButton;
import androidx.core.content.ContextCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

import javax.crypto.Cipher;

public class OnboardingFingerprintUnlockFragment extends BaseOnboardingWalletFragment {
    private boolean mUseFingerprintUnlockClicked;
    private AppCompatButton mUseFingerprintUnlockButton;
    private TextView mSkip;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mUseFingerprintUnlockClicked = false;
        return inflater.inflate(R.layout.fragment_fingerprint_unlock, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mSkip = view.findViewById(R.id.skip);
        mSkip.setOnClickListener(
                v -> {
                    if (mUseFingerprintUnlockClicked) {
                        return;
                    }
                    mUseFingerprintUnlockClicked = true;
                    goToTheNextPage();
                });

        mUseFingerprintUnlockButton = view.findViewById(R.id.button_fingerprint_unlock_continue);
        mUseFingerprintUnlockButton.setOnClickListener(
                v -> {
                    if (mUseFingerprintUnlockClicked) {
                        return;
                    }
                    mUseFingerprintUnlockClicked = true;

                    final Cipher cipher = KeystoreHelper.getCipherForEncryption();
                    if (Utils.isBiometricSupported(requireContext()) && cipher != null) {
                        // noinspection NewApi
                        setUpBiometric(mOnboardingViewModel.getPassword(), cipher);
                    } else {
                        goToTheNextPage();
                    }
                });
    }

    @Override
    public void onResume() {
        super.onResume();
        mUseFingerprintUnlockClicked = false;
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void setUpBiometric(@NonNull final String password, @NonNull final Cipher cipher) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        Cipher resultCipher = result.getCryptoObject().getCipher();
                        assert resultCipher != null;
                        KeystoreHelper.useBiometricOnUnlock(password, resultCipher);
                        goToTheNextPage();
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);
                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(
                                            ContextUtils.getApplicationContext(),
                                            errString,
                                            Toast.LENGTH_SHORT)
                                    .show();
                        }
                        mUseFingerprintUnlockClicked = false;
                    }
                };
        Executor executor = ContextCompat.getMainExecutor(requireContext());
        new BiometricPrompt.Builder(requireContext())
                .setTitle(getResources().getString(R.string.enable_fingerprint_unlock))
                .setDescription(getResources().getString(R.string.enable_fingerprint_text))
                .setNegativeButton(
                        getResources().getString(android.R.string.cancel),
                        executor,
                        (dialog, which) ->
                                authenticationCallback.onAuthenticationError(
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(
                        new BiometricPrompt.CryptoObject(cipher),
                        new CancellationSignal(),
                        executor,
                        authenticationCallback);
    }

    private void goToTheNextPage() {
        if (mOnNextPage != null) {
            mOnNextPage.incrementPages(1);
        }
    }
}
