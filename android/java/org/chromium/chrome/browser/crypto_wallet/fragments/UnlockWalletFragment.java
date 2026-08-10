/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.base.BraveClipboardHelper;

import javax.crypto.Cipher;

@NullMarked
public class UnlockWalletFragment extends BaseWalletNextPageFragment
        implements BaseWalletNextPageFragment.BiometricAuthenticationCallback {

    private TextInputEditText mUnlockWalletPassword;
    private TextInputLayout mUnlockWalletPasswordLayout;
    private AppCompatButton mUnlockButton;
    private TextView mUnlockWalletRestoreButton;
    private ImageView mBiometricUnlockButton;
    @Nullable private Cipher mCipher;
    private OnboardingViewModel mOnboardingViewModel;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
        mCipher = KeystoreHelper.getCipherForDecryption();
        // Shared with the host activity; survives configuration changes, so it carries the unlock
        // password and the biometric prompt dismissal across a rotation (the fragment is recreated
        // fresh).
        mOnboardingViewModel =
                new ViewModelProvider(requireActivity()).get(OnboardingViewModel.class);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_unlock_wallet, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.unlock_wallet_root));

        mUnlockWalletPassword = view.findViewById(R.id.unlock_wallet_password);
        mUnlockWalletPasswordLayout = view.findViewById(R.id.unlock_wallet_password_layout);
        mUnlockButton = view.findViewById(R.id.btn_unlock);
        mUnlockWalletRestoreButton = view.findViewById(R.id.btn_unlock_wallet_restore);
        mBiometricUnlockButton = view.findViewById(R.id.biometric_unlock_wallet);

        // Align the header text to the start in the two-column layout.
        if (view.findViewById(R.id.column_divider) != null) {
            ((TextView) view.findViewById(R.id.unlock_wallet_title)).setGravity(Gravity.START);
            ((TextView) view.findViewById(R.id.unlock_wallet_subtitle)).setGravity(Gravity.START);
        }

        final OnNextPage onNextPage = mOnNextPage;
        if (onNextPage == null) {
            // mOnNextPage might be {@code null} when detached from the screen.
            // It's very unlikely to happen during on view creation but better be extra
            // safe and return immediately.
            return;
        }

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
                        mOnboardingViewModel.setUnlockPassword(text.toString());
                    }

                    @Override
                    public void afterTextChanged(Editable text) {
                        /* Not used. */
                    }
                });

        // Restore the password entered before a configuration change such as a rotation.
        final String savedPassword = mOnboardingViewModel.getUnlockPassword();
        if (savedPassword != null) {
            mUnlockWalletPassword.setText(savedPassword);
            mUnlockWalletPassword.setSelection(savedPassword.length());
        }

        mUnlockButton.setOnClickListener(
                v -> {
                    final KeyringService keyringService = getKeyringService();
                    final Editable passwordText = mUnlockWalletPassword.getText();
                    if (keyringService != null && passwordText != null) {
                        final String password = passwordText.toString();
                        keyringService.unlock(
                                password,
                                result -> {
                                    if (result) {
                                        BraveClipboardHelper.clearClipboard(password);
                                        mUnlockWalletPassword.setText(null);
                                        mOnboardingViewModel.clearUnlockState();
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
                    mUnlockWalletRestoreButton.setEnabled(false);
                    onNextPage.gotoRestorePage(false);
                    mUnlockWalletPassword.setText(null);
                });

        final Cipher cipher = mCipher;
        mBiometricUnlockButton.setOnClickListener(
                v -> {
                    if (Utils.isBiometricSupported(requireContext()) && cipher != null) {
                        // Tapping the button is an explicit request for the prompt, so clear any
                        // earlier dismissal.
                        mOnboardingViewModel.setBiometricPromptDismissed(false);
                        showBiometricAuthenticationDialog(mBiometricUnlockButton, this, cipher);
                    }
                });

        if (KeystoreHelper.shouldUseBiometricToUnlock()
                && Utils.isBiometricSupported(requireContext())
                && cipher != null) {

            mBiometricUnlockButton.setVisibility(View.VISIBLE);
            // Skip the automatic prompt if the user already dismissed it for this unlock screen.
            if (!mOnboardingViewModel.isBiometricPromptDismissed()) {
                showBiometricAuthenticationDialog(mBiometricUnlockButton, this, cipher);
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mUnlockWalletRestoreButton.setEnabled(true);
        if (mOnNextPage != null) {
            mOnNextPage.showCloseButton(false);
            mOnNextPage.showBackButton(false);
        }
    }

    @Override
    public void authenticationSuccess(String unlockWalletPassword) {
        BraveClipboardHelper.clearClipboard(unlockWalletPassword);
        mUnlockWalletPassword.setText(null);
        mOnboardingViewModel.clearUnlockState();
        if (mOnNextPage != null) {
            mOnNextPage.showWallet(false);
        }
    }

    @Override
    public void authenticationDismissed() {
        // Remember the dismissal so a configuration change such as a rotation does not bring the
        // biometric prompt back up automatically.
        mOnboardingViewModel.setBiometricPromptDismissed(true);
    }
}
