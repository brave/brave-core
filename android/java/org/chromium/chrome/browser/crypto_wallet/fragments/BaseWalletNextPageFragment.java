/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import static android.hardware.biometrics.BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.CancellationSignal;
import android.text.TextUtils;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
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
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;

/**
 * Base Brave Wallet fragment that performs a cast on the host activity to extract {@link
 * OnNextPage} interface used for basic navigation actions.
 */
public abstract class BaseWalletNextPageFragment extends Fragment {
    public interface BiometricAuthenticationCallback {
        void authenticationSuccess(@NonNull final String unlockWalletPassword);
    }

    // Might be {@code null} when detached from the screen.
    @Nullable protected OnNextPage mOnNextPage;

    @Nullable private AnimationDrawable mAnimationDrawable;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        try {
            mOnNextPage = (OnNextPage) context;
        } catch (ClassCastException e) {
            throw new ClassCastException("Host activity must implement OnNextPage interface.");
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mAnimationDrawable != null) {
            mAnimationDrawable.start();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mAnimationDrawable != null) {
            mAnimationDrawable.stop();
        }
    }

    @Override
    public void onDetach() {
        mOnNextPage = null;
        super.onDetach();
    }

    @Nullable
    protected NetworkModel getNetworkModel() {
        Activity activity = requireActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getNetworkModel();
        }

        return null;
    }

    @Nullable
    protected KeyringModel getKeyringModel() {
        Activity activity = requireActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringModel();
        }

        return null;
    }

    @Nullable
    protected KeyringService getKeyringService() {
        Activity activity = requireActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    @Nullable
    protected JsonRpcService getJsonRpcService() {
        Activity activity = requireActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getJsonRpcService();
        }

        return null;
    }

    @Nullable
    protected BraveWalletP3a getBraveWalletP3A() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletP3A();
        }

        return null;
    }

    protected void setAnimatedBackground(@NonNull final View rootView) {
        mAnimationDrawable =
                (AnimationDrawable)
                        ContextCompat.getDrawable(
                                requireContext(), R.drawable.onboarding_gradient_animation);
        if (mAnimationDrawable != null) {
            rootView.setBackground(mAnimationDrawable);
            mAnimationDrawable.setEnterFadeDuration(10);
            mAnimationDrawable.setExitFadeDuration(5000);
        }
    }

    /** Shows biometric authentication button if supported and if it was previously set. */
    protected void showBiometricAuthenticationButton(@NonNull final View view) {
        if (Utils.isBiometricSupported(requireContext())
                && KeystoreHelper.shouldUseBiometricToUnlock()) {
            view.setVisibility(View.VISIBLE);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.P)
    protected void showBiometricAuthenticationDialog(
            @NonNull final View biometricUnlockButton,
            @NonNull final BiometricAuthenticationCallback biometricAuthenticationCallback,
            @NonNull final Cipher cipher) {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult authenticationResult) {
                        super.onAuthenticationSucceeded(authenticationResult);

                        final KeyringService keyringService = getKeyringService();
                        String unlockWalletPassword;
                        final Cipher resultCipher =
                                authenticationResult.getCryptoObject().getCipher();
                        assert resultCipher != null;

                        try {
                            unlockWalletPassword = KeystoreHelper.decryptText(resultCipher);
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
                            showBiometricAuthenticationButton(biometricUnlockButton);
                            return;
                        }

                        if (TextUtils.isEmpty(unlockWalletPassword) || keyringService == null) {
                            showBiometricAuthenticationButton(biometricUnlockButton);
                            return;
                        }

                        keyringService.unlock(
                                unlockWalletPassword,
                                result -> {
                                    if (result) {
                                        biometricAuthenticationCallback.authenticationSuccess(
                                                unlockWalletPassword);
                                    } else {
                                        showBiometricAuthenticationButton(biometricUnlockButton);
                                    }
                                });
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        final Context context = getContext();
                        // Error code 10 is when the user taps back to dismiss the dialog,
                        // there's no need to show a toast to log this action.
                        if (!TextUtils.isEmpty(errString) && context != null && errorCode != 10) {
                            Toast.makeText(context, errString, Toast.LENGTH_SHORT).show();
                        }
                        showBiometricAuthenticationButton(biometricUnlockButton);
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
                .authenticate(
                        new BiometricPrompt.CryptoObject(cipher),
                        new CancellationSignal(),
                        executor,
                        authenticationCallback);
    }
}
