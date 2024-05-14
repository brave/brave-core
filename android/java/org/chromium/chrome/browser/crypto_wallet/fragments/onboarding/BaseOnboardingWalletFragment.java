/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.lifecycle.ViewModelProvider;
import androidx.lifecycle.ViewModelStoreOwner;

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

    protected void enable(@NonNull final AppCompatButton button, final boolean enable) {
        if (enable) {
            button.setAlpha(1f);
            button.setEnabled(true);
        } else {
            button.setAlpha(0.5f);
            button.setEnabled(false);
        }
    }
}
