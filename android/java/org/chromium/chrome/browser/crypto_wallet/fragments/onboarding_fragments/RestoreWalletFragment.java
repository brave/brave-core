/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.app.Activity;
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
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

public class RestoreWalletFragment extends CryptoOnboardingFragment {
    private static final String IS_ONBOARDING = "is_onboarding";
    private EditText mRecoveryPhraseText;
    private EditText mPasswordEdittext;
    private EditText mRetypePasswordEdittext;
    private CheckBox mShowRecoveryPhraseCheckbox;
    private CheckBox mRestoreLegacyWalletCheckbox;
    private boolean mIsLegacyWalletRestoreEnable;
    private boolean mIsOnboarding;

    public static RestoreWalletFragment newInstance(boolean isOnboarding) {
        RestoreWalletFragment fragment = new RestoreWalletFragment();
        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING, isOnboarding);
        fragment.setArguments(args);
        return fragment;
    }

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
        mIsOnboarding = getArguments().getBoolean(IS_ONBOARDING);
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
                (buttonView, isChecked) -> {
                    mIsLegacyWalletRestoreEnable = isChecked;
                });

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

                                proceedWithAStrongPassword(passwordInput, mRecoveryPhraseText);
                            });
                });
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void enableBiometricLogin(String passwordInput) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    private void onNextPage() {
                        onNextPage.gotoNextPage(true);
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

                        // Even though we have an error, we still let to proceed
                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                        }
                        onNextPage();
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    private void proceedWithAStrongPassword(String passwordInput, EditText mRecoveryPhraseText) {
        String retypePasswordInput = mRetypePasswordEdittext.getText().toString();

        if (!passwordInput.equals(retypePasswordInput)) {
            mRetypePasswordEdittext.setError(
                    getResources().getString(R.string.retype_password_error));
        } else {
            KeyringService keyringService = getKeyringService();
            assert keyringService != null;
            keyringService.restoreWallet(
                    mRecoveryPhraseText.getText().toString().trim(),
                    passwordInput,
                    mIsLegacyWalletRestoreEnable,
                    result -> {
                        if (result) {
                            Utils.hideKeyboard(getActivity());
                            keyringService.notifyWalletBackupComplete();
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                                    && Utils.isBiometricAvailable(getContext())) {
                                // Clear previously set bio-metric credentials
                                KeystoreHelper.resetBiometric();
                                enableBiometricLogin(retypePasswordInput);
                            } else {
                                onNextPage.gotoNextPage(true);
                            }
                            Utils.setCryptoOnboarding(false);
                            Utils.clearClipboard(
                                    mRecoveryPhraseText.getText().toString().trim(), 0);
                            Utils.clearClipboard(passwordInput, 0);
                            Utils.clearClipboard(retypePasswordInput, 0);

                            cleanUp();
                        } else {
                            Toast.makeText(
                                            getActivity(),
                                            R.string.account_recovery_failed,
                                            Toast.LENGTH_SHORT)
                                    .show();
                        }
                    });
            onNextPage.gotoNextPage(false);
        }
    }

    private void cleanUp() {
        mRecoveryPhraseText.getText().clear();
        mPasswordEdittext.getText().clear();
        mRetypePasswordEdittext.getText().clear();
        mShowRecoveryPhraseCheckbox.setChecked(false);
        mRestoreLegacyWalletCheckbox.setChecked(false);
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
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }
}
