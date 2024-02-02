/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentActivity;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;

/** Onboarding fragment showing terms and conditions to accept before using Brave Wallet. */
public class OnboardingTermsOfUseFragment extends BaseOnboardingWalletFragment {
    @NonNull
    public static OnboardingTermsOfUseFragment newInstance() {
        return new OnboardingTermsOfUseFragment();
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
        return inflater.inflate(R.layout.fragment_terms_of_use_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        AppCompatButton continueButton = view.findViewById(R.id.continue_button);
        //        continueButton.setAlpha(1f);
        //        continueButton.setEnabled(true);
        continueButton.setOnClickListener(
                v -> {
                    if (mOnNextPage != null) {
                        mOnNextPage.gotoNextPage();
                    }
                });
    }
}
