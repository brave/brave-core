/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.text.Editable;
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

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class OnboardingRestoreWalletFragment extends BaseOnboardingWalletFragment {
    private EditText mRecoveryPhraseText;
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
                    String recoverPhrase = mRecoveryPhraseText.getText().toString().trim();
                    goToTheNextPage(recoverPhrase);
                });
    }

    private void goToTheNextPage(@NonNull final String recoveryPhrase) {
        mOnboardingViewModel.setRecoveryPhrase(recoveryPhrase);
        mOnboardingViewModel.setLegacyRestoreEnabled(mIsLegacyWalletRestoreEnable);

        Utils.clearClipboard(recoveryPhrase);
        mRecoveryPhraseText.getText().clear();

        if (mOnNextPage != null) {
            mOnNextPage.incrementPages(1);
        }
    }
}
