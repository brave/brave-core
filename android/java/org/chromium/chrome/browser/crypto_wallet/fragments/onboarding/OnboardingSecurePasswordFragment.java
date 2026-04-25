/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;

import com.google.android.material.textfield.TextInputEditText;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.custom_layout.PasswordStrengthMeterView;

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
                    goToTheNextPage(text.toString());
                });

        mPasswordStrengthMeterView = view.findViewById(R.id.password_strength_meter);
        mPasswordStrengthMeterView.setListener(match -> mContinueButton.setEnabled(match));
    }

    @Override
    public void onResume() {
        super.onResume();
        mCreateWalletClicked = false;
    }

    private void goToTheNextPage(@NonNull final String passwordInput) {
        mOnboardingViewModel.setPassword(passwordInput);
        if (mOnNextPage != null) {
            // If biometric authentication is not supported, we should skip
            // the fingerprint setup screen, incrementing by 2 pages.
            final int numberOfPages = Utils.isBiometricSupported(requireContext()) ? 1 : 2;
            mOnNextPage.incrementPages(numberOfPages);
        }
    }
}
