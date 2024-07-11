/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;

public class OnboardingVerifyRecoveryPhraseFragment extends BaseOnboardingWalletFragment {
    public enum VerificationStep {
        FIRST,
        SECOND,
        THIRD
    }
    private static final String IS_ONBOARDING_ARG = "isOnboarding";
    private static final String VERIFICATION_STEP_ARG = "verificationStep";

    private Button mRecoveryPhraseButton;
    private TextView mSkip;
    private TextView mCheckWord;
    private TextInputLayout mTextInputLayout;
    private TextInputEditText mTextInputExitText;
    private boolean mIsOnboarding;
    private VerificationStep mVerificationStep;
    private Pair<Integer, String> mWordToMatch;
    private int mWordPosition;
    private String mPhraseNotMatch;

    public interface OnRecoveryPhraseSelected {
        void onSelectedRecoveryPhrase(String phrase);
    }

    @NonNull
    public static OnboardingVerifyRecoveryPhraseFragment newInstance(final boolean isOnboarding, @NonNull final VerificationStep verificationStep) {
        OnboardingVerifyRecoveryPhraseFragment fragment = new OnboardingVerifyRecoveryPhraseFragment();

        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING_ARG, isOnboarding);
        args.putSerializable(VERIFICATION_STEP_ARG, verificationStep);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Bundle bundle = requireArguments();
        mIsOnboarding = bundle.getBoolean(IS_ONBOARDING_ARG, false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            mVerificationStep = bundle.getSerializable(VERIFICATION_STEP_ARG, VerificationStep.class);
        } else {
            mVerificationStep = (VerificationStep) bundle.getSerializable(VERIFICATION_STEP_ARG);
        }
        if (mVerificationStep != null) {
            mWordToMatch = mOnboardingViewModel.getVerificationStep(mVerificationStep);
        }
        mPhraseNotMatch = getResources().getString(R.string.phrase_does_not_match);
    }

    @Override
    public View onCreateView(
           @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mCheckWord = view.findViewById(R.id.check_word);
        mCheckWord.setText(getString(R.string.enter_word, mWordToMatch.first));
        mTextInputLayout = view.findViewById(R.id.text_input_layout);
        mTextInputExitText = view.findViewById(R.id.text_input_edit_text);
        mRecoveryPhraseButton = view.findViewById(R.id.button_verify_word);
        mRecoveryPhraseButton.setOnClickListener(
                v -> {
                    final Editable typedWord = mTextInputExitText.getText();
                    if (typedWord == null) {
                        phraseNotMatch();
                        return;
                    }
                    if (mWordToMatch.second.equals(typedWord.toString())) {
                        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                        KeyringService keyringService = getKeyringService();
                        if (braveWalletP3A != null && mIsOnboarding) {
                            braveWalletP3A.reportOnboardingAction(OnboardingAction.COMPLETE);
                        }
                        if (keyringService != null) {
                            keyringService.notifyWalletBackupComplete();
                        }
                        if (mOnNextPage != null) {
                            if (mIsOnboarding) {
                                // Show confirmation screen
                                // only during onboarding process.
                                mOnNextPage.incrementPages(1);
                            } else {
                                // It's important to call the
                                // `showWallet` method instead of
                                // finishing the activity as we need to
                                // reload the portfolio section to hide the
                                // backup banner.
                                mOnNextPage.showWallet(true);
                            }
                        }
                    } else {
                        phraseNotMatch();
                    }
                });
        mSkip = view.findViewById(R.id.skip);
        mSkip.setOnClickListener(
                v -> {
                    BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                    if (braveWalletP3A != null && mIsOnboarding) {
                        braveWalletP3A.reportOnboardingAction(
                                OnboardingAction.COMPLETE_RECOVERY_SKIPPED);
                    }
                    if (mIsOnboarding) {
                        if (mOnNextPage != null) {
                            // Show confirmation screen
                            // only during onboarding process.
                            mOnNextPage.incrementPages(1);
                        }
                    } else {
                        requireActivity().finish();
                    }
                });
    }

    private void phraseNotMatch() {
        mTextInputLayout.setError(mPhraseNotMatch);
    }
}
