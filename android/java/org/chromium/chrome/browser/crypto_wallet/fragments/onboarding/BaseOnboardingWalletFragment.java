/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.graphics.drawable.AnimationDrawable;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.core.content.ContextCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.BaseWalletNextPageFragment;

/**
 * Base abstract class used by onboarding fragments holding a reference to next page interface and
 * containing two abstract methods that define navigation behavior.
 *
 * @see org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage
 */
public abstract class BaseOnboardingWalletFragment extends BaseWalletNextPageFragment {

    /** Returns {@code true} if the fragment can be closed. */
    protected boolean canBeClosed() {
        return true;
    }

    /** Returns {@code true} if the fragment allows backward navigation. */
    protected boolean canNavigateBack() {
        return true;
    }

    @Nullable private AnimationDrawable mAnimationDrawable;

    @Override
    public void onResume() {
        super.onResume();
        if (mOnNextPage != null) {
            // Show or hide close icon depending on the fragment configuration.
            mOnNextPage.showCloseButton(canBeClosed());
            // Show or hide back icon depending on the fragment configuration.
            mOnNextPage.showBackButton(canNavigateBack());
        }
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
