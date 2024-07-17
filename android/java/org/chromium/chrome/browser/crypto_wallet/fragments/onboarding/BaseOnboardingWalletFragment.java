/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.AppCompatButton;
import androidx.lifecycle.ViewModelProvider;
import androidx.lifecycle.ViewModelStoreOwner;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.BaseWalletNextPageFragment;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;

/**
 * Base abstract class used by onboarding fragments holding a reference to next page interface and
 * containing two abstract methods that define navigation behavior.
 *
 * @see org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage
 */
public abstract class BaseOnboardingWalletFragment extends BaseWalletNextPageFragment {

    protected OnboardingViewModel mOnboardingViewModel;
    private AlertDialog mDialog;

    /** Returns {@code true} if the fragment can be closed. */
    protected boolean canBeClosed() {
        return true;
    }

    /** Returns {@code true} if the fragment allows backward navigation. */
    protected boolean canNavigateBack() {
        return true;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mOnboardingViewModel =
                new ViewModelProvider((ViewModelStoreOwner) requireActivity())
                        .get(OnboardingViewModel.class);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mOnNextPage != null) {
            // Show or hide close icon depending on the fragment configuration.
            mOnNextPage.showCloseButton(canBeClosed());
            // Show or hide back icon depending on the fragment configuration.
            mOnNextPage.showBackButton(canNavigateBack());
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mDialog != null && mDialog.isShowing()) {
            mDialog.dismiss();
        }
    }

    protected void showSkipDialog(final boolean isOnboarding, final int incrementCount) {
        MaterialAlertDialogBuilder builder =
                new MaterialAlertDialogBuilder(
                                requireContext(), R.style.BraveWalletAlertDialogTheme)
                        .setView(R.layout.dialog_skip_onboarding);
        mDialog = builder.show();
        AppCompatButton goBack = mDialog.findViewById(R.id.button_go_back);
        if (goBack != null) {
            goBack.setOnClickListener(v -> mDialog.dismiss());
        }
        TextView skip = mDialog.findViewById(R.id.skip);
        if (skip != null) {
            skip.setOnClickListener(
                    v -> {
                        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                        if (braveWalletP3A != null && isOnboarding) {
                            braveWalletP3A.reportOnboardingAction(
                                    OnboardingAction.COMPLETE_RECOVERY_SKIPPED);
                        }
                        if (isOnboarding) {
                            if (mOnNextPage != null) {
                                // Show confirmation screen
                                // only during onboarding process.
                                mOnNextPage.incrementPages(incrementCount);
                            }
                        } else {
                            requireActivity().finish();
                        }
                    });
        }
    }
}
