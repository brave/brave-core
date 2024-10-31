/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;

public class OnboardingVerifyRecoveryPhraseFragment extends BaseOnboardingWalletFragment {
    public enum VerificationStep {
        FIRST(0),
        SECOND(1),
        THIRD(2);

        final int mStep;

        VerificationStep(final int step) {
            mStep = step;
        }

        public int getValue() {
            return mStep;
        }
    }

    private static final String IS_ONBOARDING_ARG = "isOnboarding";
    private static final String VERIFICATION_STEP_ARG = "verificationStep";

    private AppCompatButton mRecoveryPhraseButton;
    private TextView mSkip;
    private TextView mCheckWord;
    private TextInputLayout mTextInputLayout;
    private TextInputEditText mTextInputEditText;
    private ImageView mStep1;
    private ImageView mStep2;
    private ImageView mStep3;
    @DrawableRes private int mCurrentStepRes;
    private boolean mIsOnboarding;
    private VerificationStep mVerificationStep;
    private Pair<Integer, String> mWordToMatch;
    private String mWordMismatch;

    @NonNull
    public static OnboardingVerifyRecoveryPhraseFragment newInstance(
            final boolean isOnboarding, @NonNull final VerificationStep verificationStep) {
        OnboardingVerifyRecoveryPhraseFragment fragment =
                new OnboardingVerifyRecoveryPhraseFragment();

        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING_ARG, isOnboarding);
        args.putSerializable(VERIFICATION_STEP_ARG, verificationStep);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    @Override
    protected boolean canBeClosed() {
        return false;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Bundle bundle = requireArguments();
        mIsOnboarding = bundle.getBoolean(IS_ONBOARDING_ARG, false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            mVerificationStep =
                    bundle.getSerializable(VERIFICATION_STEP_ARG, VerificationStep.class);
        } else {
            mVerificationStep = (VerificationStep) bundle.getSerializable(VERIFICATION_STEP_ARG);
        }
        mWordMismatch = getResources().getString(R.string.phrase_does_not_match);
        mCurrentStepRes = R.drawable.rectangle_selected_9;
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRecoveryPhraseButton = view.findViewById(R.id.button_verify_word);
        mCheckWord = view.findViewById(R.id.check_word);
        mTextInputLayout = view.findViewById(R.id.text_input_layout);
        mTextInputEditText = view.findViewById(R.id.text_input_edit_text);
        mStep1 = view.findViewById(R.id.recovery_step_1);
        mStep2 = view.findViewById(R.id.recovery_step_2);
        mStep3 = view.findViewById(R.id.recovery_step_3);

        if (mVerificationStep == VerificationStep.FIRST) {
            mStep1.setImageResource(mCurrentStepRes);
        } else if (mVerificationStep == VerificationStep.SECOND) {
            mStep2.setImageResource(mCurrentStepRes);
        } else if (mVerificationStep == VerificationStep.THIRD) {
            mStep3.setImageResource(mCurrentStepRes);
        }

        mTextInputEditText.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                        /* Not used. */
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        mRecoveryPhraseButton.setEnabled(s.length() != 0);
                        mTextInputLayout.setErrorEnabled(false);
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                        /* Not used. */
                    }
                });

        mRecoveryPhraseButton.setOnClickListener(
                v -> {
                    if (mWordToMatch == null) {
                        return;
                    }
                    final Editable typedWord = mTextInputEditText.getText();
                    if (typedWord == null) {
                        wordMismatch();
                        return;
                    }
                    if (mWordToMatch.second.equals(typedWord.toString())) {
                        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                        KeyringService keyringService = getKeyringService();
                        if (mVerificationStep == VerificationStep.THIRD) {
                            if (mIsOnboarding && braveWalletP3A != null) {
                                braveWalletP3A.reportOnboardingAction(OnboardingAction.COMPLETE);
                            }
                            if (keyringService != null) {
                                keyringService.notifyWalletBackupComplete();
                            }
                        }
                        if (mOnNextPage != null) {
                            if (!mIsOnboarding && mVerificationStep == VerificationStep.THIRD) {
                                // It's important to call the
                                // `showWallet` method instead of
                                // finishing the activity as we need to
                                // reload the portfolio section to hide the
                                // backup banner.
                                mOnNextPage.showWallet(true);
                            } else {
                                // Show confirmation screen
                                // only during onboarding process.
                                mOnNextPage.incrementPages(1);
                            }
                        }
                    } else {
                        wordMismatch();
                    }
                });
        mSkip = view.findViewById(R.id.skip);
        mSkip.setOnClickListener(
                v -> showSkipDialog(mIsOnboarding, 3 - mVerificationStep.getValue()));
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mVerificationStep != null && mWordToMatch == null) {
            mWordToMatch = mOnboardingViewModel.getVerificationStep(mVerificationStep);
            if (mWordToMatch != null) {
                mCheckWord.setText(getString(R.string.enter_word, mWordToMatch.first + 1));
            }
        }
    }

    private void wordMismatch() {
        mTextInputLayout.setError(mWordMismatch);
    }
}
