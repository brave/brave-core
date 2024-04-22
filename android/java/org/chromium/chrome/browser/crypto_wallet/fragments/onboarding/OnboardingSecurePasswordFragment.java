/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.annotation.SuppressLint;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.widget.AppCompatButton;
import androidx.core.content.ContextCompat;

import com.google.android.material.textfield.TextInputEditText;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.custom_layout.PasswordStrengthMeterView;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

public class OnboardingSecurePasswordFragment extends BaseOnboardingWalletFragment {
    private boolean mCreateWalletClicked;
    private PasswordStrengthMeterView mPasswordStrengthMeterView;
    private AppCompatButton mContinueButton;
    private TextInputEditText mRetypePasswordEditText;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mCreateWalletClicked = false;
        return inflater.inflate(R.layout.fragment_secure_password, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRetypePasswordEditText = view.findViewById(R.id.text_input_retype_edit_text);
        mContinueButton = view.findViewById(R.id.btn_secure_crypto_continue);
        mContinueButton.setOnClickListener(
                v -> {
                    Editable text = mRetypePasswordEditText.getText();
                    if (mCreateWalletClicked || text == null) {
                        return;
                    }
                    mCreateWalletClicked = true;
                    proceedWithStrongPassword(text.toString());
                });

        mPasswordStrengthMeterView = view.findViewById(R.id.password_strength_meter);
        mPasswordStrengthMeterView.setListener(match -> enable(mContinueButton, match));
    }

    @Override
    public void onResume() {
        super.onResume();
        mCreateWalletClicked = false;
    }

    private void proceedWithStrongPassword(@NonNull final String password) {
        if (Utils.isBiometricSupported(requireContext())) {
            // noinspection NewApi
            setUpBiometric(password);
        } else {
            goToTheNextPage(password);
        }
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void setUpBiometric(@NonNull final String password) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        KeystoreHelper.useBiometricOnUnlock(password);
                        goToTheNextPage(password);
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);
                        // Even though we have an error, we still let to proceed
                        Toast.makeText(
                                        ContextUtils.getApplicationContext(),
                                        errString,
                                        Toast.LENGTH_SHORT)
                                .show();
                        goToTheNextPage(password);
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
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    private void goToTheNextPage(@NonNull final String passwordInput) {
        mOnboardingViewModel.setPassword(passwordInput);
        if (mOnNextPage != null) {
            mOnNextPage.gotoNextPage();
        }
    }
}
