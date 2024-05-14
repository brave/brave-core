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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.concurrent.Executor;

public class OnboardingBackupWalletFragment extends BaseOnboardingWalletFragment {
    private static final String IS_ONBOARDING_ARG = "isOnboarding";

    private boolean mIsOnboarding;
    private TextView mBackupWalletTitle;
    private EditText mBackupWalletPassword;
    private ImageView mBiometricBackupWalletImage;
    private CheckBox mBackupWalletCheckbox;
    private Button mBackupWalletButton;
    private String mPasswordFromBiometric;
    private boolean mBiometricExecuted;

    @NonNull
    public static OnboardingBackupWalletFragment newInstance(final boolean isOnboarding) {
        OnboardingBackupWalletFragment fragment = new OnboardingBackupWalletFragment();

        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING_ARG, isOnboarding);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mIsOnboarding = requireArguments().getBoolean(IS_ONBOARDING_ARG, false);
        return inflater.inflate(R.layout.fragment_backup_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mPasswordFromBiometric = "";
        mBackupWalletTitle = view.findViewById(R.id.tv_backup_wallet_password_title);
        mBackupWalletPassword = view.findViewById(R.id.et_backup_wallet_password);
        mBiometricBackupWalletImage = view.findViewById(R.id.iv_biometric_unlock_wallet);
        mBackupWalletButton = view.findViewById(R.id.btn_backup_wallet_continue);
        mBackupWalletCheckbox = view.findViewById(R.id.backup_wallet_checkbox);

        mBackupWalletPassword.addTextChangedListener(new FilterTextWatcherPassword());
        mBackupWalletButton.setOnClickListener(
                v -> {
                    BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                    if (mIsOnboarding) {
                        if (mOnNextPage != null) {
                            mOnNextPage.gotoNextPage();
                        }
                        if (braveWalletP3A != null) {
                            braveWalletP3A.reportOnboardingAction(OnboardingAction.RECOVERY_SETUP);
                        }
                        return;
                    }
                    KeyringService keyringService = getKeyringService();
                    if (keyringService != null) {
                        final String passwordToUse =
                                mPasswordFromBiometric.isEmpty()
                                        ? mBackupWalletPassword.getText().toString()
                                        : mPasswordFromBiometric;
                        keyringService.getWalletMnemonic(
                                passwordToUse,
                                result -> {
                                    if (result != null && !result.isEmpty()) {
                                        mOnboardingViewModel.setPassword(passwordToUse);
                                        if (mOnNextPage != null) {
                                            mOnNextPage.gotoNextPage();
                                        }
                                    } else {
                                        showPasswordRelatedControls(true);
                                        mBackupWalletPassword.setError(
                                                getString(R.string.incorrect_password_error));
                                    }
                                });
                    }
                });
        mBackupWalletCheckbox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> {
                    if (!mIsOnboarding
                            && !mBiometricExecuted
                            && Utils.isBiometricSupported(getContext())
                            && KeystoreHelper.shouldUseBiometricToUnlock()) {
                        // noinspection NewApi
                        createBiometricPrompt();

                        return;
                    }
                    enableDisableContinueButton(isChecked);
                });
        TextView backupWalletSkipButton = view.findViewById(R.id.btn_backup_wallet_skip);
        backupWalletSkipButton.setOnClickListener(
                v -> {
                    BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                    if (braveWalletP3A != null && mIsOnboarding) {
                        braveWalletP3A.reportOnboardingAction(
                                OnboardingAction.COMPLETE_RECOVERY_SKIPPED);
                    }
                    if (mOnNextPage != null) {
                        mOnNextPage.onboardingCompleted();
                    }
                });
        mBiometricBackupWalletImage.setOnClickListener(
                v -> {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                            && Utils.isBiometricSupported(getContext())) {
                        showPasswordRelatedControls(false);
                        createBiometricPrompt();
                    }
                });
        checkOnBiometric();
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    private void checkOnBiometric() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P
                || !KeystoreHelper.shouldUseBiometricToUnlock()
                || !Utils.isBiometricSupported(getContext())) {
            showPasswordRelatedControls(true);
        }
    }

    private void showPasswordRelatedControls(boolean show) {
        if (mIsOnboarding) return;

        int visibility = show ? View.VISIBLE : View.GONE;
        mBackupWalletTitle.setVisibility(visibility);
        mBackupWalletPassword.setVisibility(visibility);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                && Utils.isBiometricSupported(getContext())
                && KeystoreHelper.shouldUseBiometricToUnlock()) {
            mBiometricBackupWalletImage.setVisibility(visibility);
        }
    }

    private void enableDisableContinueButton(boolean isChecked) {
        if (isChecked
                && (mIsOnboarding || !TextUtils.isEmpty(mBackupWalletPassword.getText())
                        || !mPasswordFromBiometric.isEmpty())) {
            mBackupWalletButton.setEnabled(true);
            mBackupWalletButton.setAlpha(1.0f);
        } else {
            mBackupWalletButton.setEnabled(false);
            mBackupWalletButton.setAlpha(0.5f);
        }
    }

    private class FilterTextWatcherPassword implements TextWatcher {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            enableDisableContinueButton(mBackupWalletCheckbox.isChecked());
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void createBiometricPrompt() {
        mBiometricExecuted = true;
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        // We authenticated using fingerprint
                        try {
                            mPasswordFromBiometric = KeystoreHelper.decryptText();
                            if (mPasswordFromBiometric.isEmpty()) {
                                showPasswordRelatedControls(true);

                                return;
                            }
                            enableDisableContinueButton(mBackupWalletCheckbox.isChecked());
                        } catch (Exception exc) {
                            showPasswordRelatedControls(true);
                        }
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                        }
                        // Even though we have an error, we still let to proceed
                        showPasswordRelatedControls(true);
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    @SuppressLint("MissingPermission")
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
}
