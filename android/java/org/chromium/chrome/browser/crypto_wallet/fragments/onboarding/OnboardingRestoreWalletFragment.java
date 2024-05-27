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

import org.chromium.base.ContextUtils;
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
        restoreWalletCopyImage.setOnClickListener(
                v -> mRecoveryPhraseText.setText(Utils.getTextFromClipboard(requireContext())));

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
                            CharSequence charSequence, int i, int i1, int i2) {
                        /* Unused. */
                    }

                    @Override
                    public void afterTextChanged(Editable editable) {
                        /* Unused. */
                    }

                    @Override
                    public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                        String recoveryPhrase = charSequence.toString().trim();

                        // Recovery phrase contains only strings, not JSON and length is 24.
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
                    String retypePasswordInput = mRetypePasswordEdittext.getText().toString();
                    String recoverPhrase = mRecoveryPhraseText.getText().toString().trim();
                    if (TextUtils.isEmpty(passwordInput) || passwordInput.length() < 8) {
                        mPasswordEdittext.setError(
                                getResources().getString(R.string.password_text));
                    } else if (!passwordInput.equals(retypePasswordInput)) {
                        mRetypePasswordEdittext.setError(
                                getResources().getString(R.string.retype_password_error));
                    } else {
                        proceedWithStrongPassword(passwordInput, recoverPhrase);
                    }
                });
    }

    private void proceedWithStrongPassword(
            @NonNull final String password, @NonNull final String recoveryPhrase) {
        if (Utils.isBiometricSupported(requireContext())) {
            // Clear previously set bio-metric credentials
            KeystoreHelper.resetBiometric();
            // noinspection NewApi
            setUpBiometric(password, recoveryPhrase);
        } else {
            goToTheNextPage(password, recoveryPhrase);
        }
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    private void setUpBiometric(
            @NonNull final String password, @NonNull final String recoveryPhrase) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        KeystoreHelper.useBiometricOnUnlock(password);
                        goToTheNextPage(password, recoveryPhrase);
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);
                        // Even though we have an error, we still let to proceed
                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(
                                            ContextUtils.getApplicationContext(),
                                            errString,
                                            Toast.LENGTH_SHORT)
                                    .show();
                        }
                        goToTheNextPage(password, recoveryPhrase);
                    }
                };
        Executor executor = ContextCompat.getMainExecutor(requireContext());
        new BiometricPrompt.Builder(getActivity())
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

    private void goToTheNextPage(
            @NonNull final String password, @NonNull final String recoveryPhrase) {
        KeyringService keyringService = getKeyringService();
        assert keyringService != null;
        keyringService.restoreWallet(
                recoveryPhrase,
                password,
                mIsLegacyWalletRestoreEnable,
                result -> {
                    if (result) {
                        Utils.hideKeyboard(requireActivity());
                        keyringService.notifyWalletBackupComplete();

                        Utils.setCryptoOnboarding(false);
                        Utils.clearClipboard(recoveryPhrase);
                        Utils.clearClipboard(password);

                        mRecoveryPhraseText.getText().clear();
                        mPasswordEdittext.getText().clear();
                        mRetypePasswordEdittext.getText().clear();
                        mShowRecoveryPhraseCheckbox.setChecked(false);
                        mRestoreLegacyWalletCheckbox.setChecked(false);

                        // Set ETH account by default as initial state.
                        keyringService.getAllAccounts(
                                allAccounts ->
                                        keyringService.setSelectedAccount(
                                                allAccounts.ethDappSelectedAccount.accountId,
                                                success -> {
                                                    if (mOnNextPage != null) {
                                                        mOnNextPage.onboardingCompleted();
                                                    }
                                                }));
                    } else {
                        Toast.makeText(
                                        requireActivity(),
                                        R.string.account_recovery_failed,
                                        Toast.LENGTH_SHORT)
                                .show();
                    }
                });
    }
}
