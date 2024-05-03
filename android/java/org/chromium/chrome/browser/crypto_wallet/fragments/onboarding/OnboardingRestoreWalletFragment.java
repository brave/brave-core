/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.annotation.SuppressLint;
import android.content.Context;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
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
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

public class OnboardingRestoreWalletFragment extends BaseOnboardingWalletFragment {
    private EditText mRecoveryPhraseText;
    private EditText mPasswordEdittext;
    private EditText mRetypePasswordEdittext;
    private CheckBox mShowRecoveryPhraseCheckbox;
    private CheckBox mRestoreLegacyWalletCheckbox;
    private boolean mIsLegacyWalletRestoreEnable;

    @NonNull
    public static OnboardingRestoreWalletFragment newInstance() {
        return new OnboardingRestoreWalletFragment();
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_restore_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRecoveryPhraseText = view.findViewById(R.id.recovery_phrase_text);
        mPasswordEdittext = view.findViewById(R.id.restore_wallet_password);
        mRetypePasswordEdittext = view.findViewById(R.id.restore_wallet_retype_password);
        mShowRecoveryPhraseCheckbox = view.findViewById(R.id.restore_wallet_checkbox);
        mRestoreLegacyWalletCheckbox = view.findViewById(R.id.restore_legacy_wallet_checkbox);

        ImageView restoreWalletCopyImage = view.findViewById(R.id.restore_wallet_copy_image);
        assert getActivity() != null;
        restoreWalletCopyImage.setOnClickListener(
                v -> mRecoveryPhraseText.setText(Utils.getTextFromClipboard(getActivity())));

        mShowRecoveryPhraseCheckbox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> {
                    int cursorPos = mRecoveryPhraseText.getSelectionStart();
                    if (isChecked) {
                        mRecoveryPhraseText.setTransformationMethod(
                                HideReturnsTransformationMethod.getInstance());
                    } else {
                        mRecoveryPhraseText.setTransformationMethod(
                                PasswordTransformationMethod.getInstance());
                    }
                    mRecoveryPhraseText.setSelection(cursorPos);
                });

        mRecoveryPhraseText.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(
                            CharSequence charSequence, int i, int i1, int i2) {}

                    @Override
                    public void afterTextChanged(Editable editable) {}

                    @Override
                    public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                        String recoveryPhrase = charSequence.toString().trim();

                        // validate recoveryPhrase contains only string. not JSON and length is 24
                        if (recoveryPhrase.matches("[a-zA-Z\\s]+")
                                && recoveryPhrase.split("\\s+").length == 24) {
                            mRestoreLegacyWalletCheckbox.setVisibility(View.VISIBLE);
                        } else {
                            mRestoreLegacyWalletCheckbox.setVisibility(View.GONE);
                        }
                    }
                });

        mRestoreLegacyWalletCheckbox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> mIsLegacyWalletRestoreEnable = isChecked);

        Button secureCryptoButton = view.findViewById(R.id.btn_restore_wallet);
        secureCryptoButton.setOnClickListener(
                v -> {
                    String passwordInput = mPasswordEdittext.getText().toString();

                    KeyringService keyringService = getKeyringService();
                    assert keyringService != null;
                    keyringService.isStrongPassword(
                            passwordInput,
                            result -> {
                                if (!result) {
                                    mPasswordEdittext.setError(
                                            getResources().getString(R.string.password_text));
                                    return;
                                }

                                proceedWithStrongPassword(passwordInput, mRecoveryPhraseText);
                            });
                });
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void enableBiometricLogin(String passwordInput) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    private void onNextPage() {
                        if (mOnNextPage != null) {
                            mOnNextPage.onboardingCompleted();
                        }
                    }

                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        KeystoreHelper.useBiometricOnUnlock(passwordInput);
                        onNextPage();
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        final Context context = getContext();
                        // Even though we have an error, we still let to proceed
                        if (!TextUtils.isEmpty(errString) && context != null) {
                            Toast.makeText(context, errString, Toast.LENGTH_SHORT).show();
                        }
                        onNextPage();
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    private void proceedWithStrongPassword(@NonNull String password, EditText recoveryPhrase) {
        String retypePasswordInput = mRetypePasswordEdittext.getText().toString();

        if (!password.equals(retypePasswordInput)) {
            mRetypePasswordEdittext.setError(
                    getResources().getString(R.string.retype_password_error));
        } else {
            KeyringService keyringService = getKeyringService();
            assert keyringService != null;
            keyringService.restoreWallet(
                    recoveryPhrase.getText().toString().trim(),
                    password,
                    mIsLegacyWalletRestoreEnable,
                    result -> {
                        if (result) {
                            Utils.hideKeyboard(requireActivity());
                            keyringService.notifyWalletBackupComplete();
                            if (Utils.isBiometricSupported(getContext())) {
                                // Clear previously set bio-metric credentials
                                KeystoreHelper.resetBiometric();
                                // noinspection NewApi
                                enableBiometricLogin(retypePasswordInput);
                            } else if (mOnNextPage != null) {
                                mOnNextPage.onboardingCompleted();
                            }
                            Utils.setCryptoOnboarding(false);
                            Utils.clearClipboard(recoveryPhrase.getText().toString().trim());
                            Utils.clearClipboard(password);
                            Utils.clearClipboard(retypePasswordInput);

                            cleanUp();
                        } else {
                            Toast.makeText(
                                            requireActivity(),
                                            R.string.account_recovery_failed,
                                            Toast.LENGTH_SHORT)
                                    .show();
                        }
                    });
            if (mOnNextPage != null) {
                mOnNextPage.gotoNextPage();
            }
        }
    }

    private void cleanUp() {
        mRecoveryPhraseText.getText().clear();
        mPasswordEdittext.getText().clear();
        mRetypePasswordEdittext.getText().clear();
        mShowRecoveryPhraseCheckbox.setChecked(false);
        mRestoreLegacyWalletCheckbox.setChecked(false);
    }

    @SuppressLint("MissingPermission")
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
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }
}
