/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.ViewModelProvider;
import androidx.lifecycle.ViewModelStoreOwner;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.concurrent.Executor;

public class SecurePasswordFragment extends CryptoOnboardingFragment {
    private boolean mCreateWalletClicked;
    private OnboardingViewModel mOnboardingViewModel;

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    private BraveWalletP3a getBraveWalletP3A() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletP3A();
        }

        return null;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mCreateWalletClicked = false;
        View view = inflater.inflate(R.layout.fragment_secure_password, container, false);
        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    Utils.hideKeyboard(getActivity());
                }
                return true;
            }
        });
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mOnboardingViewModel = new ViewModelProvider((ViewModelStoreOwner) requireActivity())
                                       .get(OnboardingViewModel.class);
        Button secureCryptoButton = view.findViewById(R.id.btn_secure_crypto_continue);
        secureCryptoButton.setOnClickListener(v -> {
            if (mCreateWalletClicked) {
                return;
            }
            mCreateWalletClicked = true;
            EditText passwordEdittext = view.findViewById(R.id.secure_crypto_password);
            String passwordInput = passwordEdittext.getText().toString();

            KeyringService keyringService = getKeyringService();
            assert keyringService != null;
            keyringService.isStrongPassword(passwordInput, result -> {
                if (!result) {
                    passwordEdittext.setError(getResources().getString(R.string.password_text));
                    mCreateWalletClicked = false;

                    return;
                }
                proceedWithAStrongPassword(passwordInput, view);
            });
        });
    }

    private void proceedWithAStrongPassword(String passwordInput, View view) {
        EditText retypePasswordEdittext = view.findViewById(R.id.secure_crypto_retype_password);
        String retypePasswordInput = retypePasswordEdittext.getText().toString();
        if (!passwordInput.equals(retypePasswordInput)) {
            retypePasswordEdittext.setError(
                    getResources().getString(R.string.retype_password_error));
            mCreateWalletClicked = false;
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                    && Utils.isBiometricAvailable(getContext())) {
                final BiometricPrompt.AuthenticationCallback authenticationCallback =
                        new BiometricPrompt.AuthenticationCallback() {
                            private void onNextPage() {
                                // Go to next Page
                                goToTheNextPage(passwordInput);
                            }

                            @Override
                            public void onAuthenticationSucceeded(
                                    BiometricPrompt.AuthenticationResult result) {
                                super.onAuthenticationSucceeded(result);
                                KeystoreHelper.useBiometricOnUnlock(passwordInput);
                                onNextPage();
                            }

                            @Override
                            public void onAuthenticationError(
                                    int errorCode, CharSequence errString) {
                                super.onAuthenticationError(errorCode, errString);

                                // Even though we have an error, we still let to proceed
                                Toast.makeText(getActivity(), errString, Toast.LENGTH_SHORT).show();
                                onNextPage();
                            }
                        };
                showFingerprintDialog(authenticationCallback);
            } else {
                goToTheNextPage(passwordInput);
            }
        }
    }

    private void goToTheNextPage(String passwordInput) {
        KeyringService keyringService = getKeyringService();
        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
        if (keyringService != null) {
            keyringService.createWallet(passwordInput, recoveryPhrases -> {
                if (braveWalletP3A != null) {
                    braveWalletP3A.reportOnboardingAction(OnboardingAction.RECOVERY_SETUP);
                }
                // Go to the next page after wallet creation is done
                Utils.setCryptoOnboarding(false);
                mOnboardingViewModel.setPassword(passwordInput);
                onNextPage.gotoNextPage(false);
            });
            showCreatingWalletPage();
        }
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

    private void showCreatingWalletPage() {
        onNextPage.gotoNextPage(false);
    }
}
