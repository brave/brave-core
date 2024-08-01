/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;

import org.chromium.chrome.R;

public class OnboardingConfirmationFragment extends BaseOnboardingWalletFragment {
    private boolean mConfirmationButtonClicked;
    private AppCompatButton mGoToPortfolioButton;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mConfirmationButtonClicked = false;
        return inflater.inflate(R.layout.fragment_confirmation_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.confirmation_wallet_root));

        mGoToPortfolioButton = view.findViewById(R.id.button_go_to_portfolio);
        mGoToPortfolioButton.setOnClickListener(
                v -> {
                    if (mConfirmationButtonClicked || mOnNextPage == null) {
                        return;
                    }
                    mConfirmationButtonClicked = true;

                    mOnNextPage.showWallet(true);
                });
    }

    @Override
    public void onResume() {
        super.onResume();
        mConfirmationButtonClicked = false;
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    @Override
    protected boolean canBeClosed() {
        return false;
    }
}
