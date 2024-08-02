/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentActivity;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.fragments.BaseWalletNextPageFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class OnboardingBackupWalletFragment extends BaseOnboardingWalletFragment
        implements BaseWalletNextPageFragment.BiometricAuthenticationCallback {
    private TextInputEditText mUnlockWalletPassword;
    private TextInputLayout mUnlockWalletPasswordLayout;
    private AppCompatButton mUnlockButton;
    private ImageView mBiometricUnlockButton;

    @NonNull
    public static OnboardingBackupWalletFragment newInstance() {
        return new OnboardingBackupWalletFragment();
    }

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
        return inflater.inflate(R.layout.fragment_backup_wallet, container, false);
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

        mUnlockWalletPassword = view.findViewById(R.id.text_input_edit_text);
        mUnlockWalletPasswordLayout = view.findViewById(R.id.text_input_layout);
        mUnlockButton = view.findViewById(R.id.button_continue);
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
                        final String passwordToUse = mUnlockWalletPassword.getText().toString();
                        keyringService.unlock(
                                passwordToUse,
                                result -> {
                                    if (result) {
                                        Utils.clearClipboard(passwordToUse);
                                        mUnlockWalletPassword.setText(null);
                                        mOnboardingViewModel.setPassword(passwordToUse);
                                        if (mOnNextPage != null) {
                                            mOnNextPage.incrementPages(1);
                                        }
                                    } else {
                                        mUnlockWalletPasswordLayout.setError(
                                                getString(R.string.incorrect_password_error));
                                    }
                                });
                    }
                });

        mBiometricUnlockButton.setOnClickListener(
                v -> {
                    if (Utils.isBiometricSupported(requireContext())) {
                        // noinspection NewApi
                        showBiometricAuthenticationDialog(mBiometricUnlockButton, this);
                    }
                });

        if (KeystoreHelper.shouldUseBiometricToUnlock()
                && Utils.isBiometricSupported(requireContext())) {

            mBiometricUnlockButton.setVisibility(View.VISIBLE);
            // noinspection NewApi
            showBiometricAuthenticationDialog(mBiometricUnlockButton, this);
        }
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    @Override
    public void authenticationSuccess(@NonNull String unlockWalletPassword) {
        Utils.clearClipboard(unlockWalletPassword);
        mUnlockWalletPassword.setText(null);
        mOnboardingViewModel.setPassword(unlockWalletPassword);
        if (mOnNextPage != null) {
            mOnNextPage.incrementPages(1);
        }
    }

    //    @SuppressLint("MissingPermission")
    //    @RequiresApi(api = Build.VERSION_CODES.P)
    //    private void showBiometricAuthenticationDialog() {
    //        final AuthenticationCallback authenticationCallback =
    //                new AuthenticationCallback() {
    //                    @Override
    //                    public void onAuthenticationSucceeded(
    //                            AuthenticationResult authenticationResult) {
    //                        super.onAuthenticationSucceeded(authenticationResult);
    //
    //                        final KeyringService keyringService = getKeyringService();
    //                        String unlockWalletPassword;
    //
    //                        try {
    //                            unlockWalletPassword = KeystoreHelper.decryptText();
    //                        } catch (InvalidAlgorithmParameterException
    //                                | UnrecoverableEntryException
    //                                | NoSuchPaddingException
    //                                | IllegalBlockSizeException
    //                                | CertificateException
    //                                | KeyStoreException
    //                                | NoSuchAlgorithmException
    //                                | BadPaddingException
    //                                | IOException
    //                                | InvalidKeyException e) {
    //                            // KeystoreHelper.decryptText() may throw a long list
    //                            // of exceptions.
    //                            showBiometricAuthenticationButton(mBiometricUnlockButton);
    //                            return;
    //                        }
    //
    //                        if (TextUtils.isEmpty(unlockWalletPassword) || keyringService == null)
    // {
    //                            showBiometricAuthenticationButton(mBiometricUnlockButton);
    //                            return;
    //                        }
    //
    //                        keyringService.unlock(
    //                                unlockWalletPassword,
    //                                result -> {
    //                                    if (result) {
    //                                        Utils.clearClipboard(unlockWalletPassword);
    //                                        mUnlockWalletPassword.setText(null);
    //
    // mOnboardingViewModel.setPassword(unlockWalletPassword);
    //                                        if (mOnNextPage != null) {
    //                                            mOnNextPage.incrementPages(1);
    //                                        }
    //                                    } else {
    //
    // showBiometricAuthenticationButton(mBiometricUnlockButton);
    //                                    }
    //                                });
    //                    }
    //
    //                    @Override
    //                    public void onAuthenticationError(int errorCode, CharSequence errString) {
    //                        super.onAuthenticationError(errorCode, errString);
    //
    //                        final Context context = getContext();
    //                        if (!TextUtils.isEmpty(errString) && context != null) {
    //                            Toast.makeText(context, errString, Toast.LENGTH_SHORT).show();
    //                        }
    //                        showBiometricAuthenticationButton(mBiometricUnlockButton);
    //                    }
    //                };
    //        Executor executor = ContextCompat.getMainExecutor(requireContext());
    //        new Builder(requireContext())
    //                .setTitle(getResources().getString(R.string.fingerprint_unlock))
    //                .setDescription(getResources().getString(R.string.use_fingerprint_text))
    //                .setNegativeButton(
    //                        getResources().getString(android.R.string.cancel),
    //                        executor,
    //                        (dialog, which) ->
    //                                authenticationCallback.onAuthenticationError(
    //                                        BIOMETRIC_ERROR_USER_CANCELED, ""))
    //                .build()
    //                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    //    }
}
