/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Build;
import android.os.Bundle;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentActivity;

import com.google.android.material.checkbox.MaterialCheckBox;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.TabUtils;

/** Onboarding fragment showing terms and conditions to accept before using Brave Wallet. */
@NullMarked
public class OnboardingTermsOfUseFragment extends BaseOnboardingWalletFragment
        implements CompoundButton.OnCheckedChangeListener {

    private MaterialCheckBox mSelfCustodyCheckBox;
    private MaterialCheckBox mTermsOfUseCheckBox;

    private AppCompatButton mContinueButton;

    private boolean mContinueButtonClicked;

    public static OnboardingTermsOfUseFragment newInstance() {
        return new OnboardingTermsOfUseFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContinueButtonClicked = false;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_terms_of_use_wallet, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mSelfCustodyCheckBox = view.findViewById(R.id.self_custody_check_box);
        mSelfCustodyCheckBox.setOnCheckedChangeListener(this);

        mTermsOfUseCheckBox = view.findViewById(R.id.terms_of_use_check_box);
        mTermsOfUseCheckBox.setOnCheckedChangeListener(this);

        Spanned termsOfUseSpanned =
                Utils.createSpanForSurroundedPhrase(
                        requireContext(),
                        R.string.accept_terms_of_use,
                        (v) -> {
                            TabUtils.openUrlInNewTab(false, Utils.BRAVE_TERMS_OF_USE_URL);
                            TabUtils.bringChromeTabbedActivityToTheTop(requireActivity());
                        });
        mTermsOfUseCheckBox.setMovementMethod(LinkMovementMethod.getInstance());
        mTermsOfUseCheckBox.setText(termsOfUseSpanned);

        mContinueButton = view.findViewById(R.id.continue_button);
        mContinueButton.setOnClickListener(
                v -> {
                    if (mContinueButtonClicked) {
                        return;
                    }
                    mContinueButtonClicked = true;

                    if (mOnNextPage != null) {
                        mOnNextPage.incrementPages(1);
                    }
                });

        // Restore the selections entered before a configuration change such as a rotation. Read
        // both values first, as setChecked triggers onCheckedChanged, which writes them back.
        final boolean selfCustodyChecked = mOnboardingViewModel.isSelfCustodyChecked();
        final boolean termsOfUseChecked = mOnboardingViewModel.isTermsOfUseChecked();
        mSelfCustodyCheckBox.setChecked(selfCustodyChecked);
        mTermsOfUseCheckBox.setChecked(termsOfUseChecked);
        updateContinueButtonState();
    }

    @Override
    public void onResume() {
        super.onResume();
        mContinueButtonClicked = false;
    }

    /**
     * Clears both terms checkboxes and refreshes the continue button state to match. Does nothing
     * when the fragment has no view.
     */
    public void uncheckSelections() {
        if (getView() == null) {
            // The view has been destroyed; it will start unchecked when recreated.
            return;
        }
        mSelfCustodyCheckBox.setChecked(false);
        mTermsOfUseCheckBox.setChecked(false);
        updateContinueButtonState();
    }

    @Override
    public void onDestroy() {
        // Drop the saved selections when the user genuinely leaves the screen (taps next, goes
        // back, or closes with X). Keep them when the fragment is destroyed for a configuration
        // change such as a rotation, which is a state restoration that repopulates the recreated
        // fragment from these values.
        final FragmentActivity activity = getActivity();
        if (activity != null && !activity.isChangingConfigurations()) {
            mOnboardingViewModel.clearTermsOfUseSelections();
        }
        super.onDestroy();
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        // Persist the selections so they survive a configuration change such as a rotation.
        mOnboardingViewModel.setTermsOfUseSelections(
                mSelfCustodyCheckBox.isChecked(), mTermsOfUseCheckBox.isChecked());
        updateContinueButtonState();
    }

    /** Enables the continue button only when both terms of use checkboxes are selected. */
    private void updateContinueButtonState() {
        mContinueButton.setEnabled(
                mSelfCustodyCheckBox.isChecked() && mTermsOfUseCheckBox.isChecked());
    }
}
