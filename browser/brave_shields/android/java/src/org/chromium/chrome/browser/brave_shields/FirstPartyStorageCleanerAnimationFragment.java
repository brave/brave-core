/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_shields;

import android.animation.Animator;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;

import com.airbnb.lottie.LottieAnimationView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

@NullMarked
public class FirstPartyStorageCleanerAnimationFragment extends DialogFragment {
    private static final String TAG = "FPStorageCleanerAnimFrag";

    private static final String ANIMATION_FILE = "brave_first_party_storage_clean_animation.json";
    private static final int FADE_OUT_DURATION_MS = 400;
    private static final int FADE_OUT_START_DELAY_MS = 0;
    private static final int ANIMATION_REPEAT_COUNT = 0;

    private @Nullable LottieAnimationView mAnimationView;
    private @Nullable View mBlackFadeOverlay;

    public static void show(Context context) {
        FragmentManager fragmentManager = ((FragmentActivity) context).getSupportFragmentManager();
        FirstPartyStorageCleanerAnimationFragment fragment =
                new FirstPartyStorageCleanerAnimationFragment();
        fragment.show(fragmentManager, TAG);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(DialogFragment.STYLE_NO_TITLE, android.R.style.Theme_Black_NoTitleBar_Fullscreen);
    }

    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);

        Window window = dialog.getWindow();
        if (window != null) {
            window.setLayout(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            window.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
            window.addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            window.addFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);
            window.addFlags(WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH);
        }

        return dialog;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(
                R.layout.brave_first_party_storage_cleaner_animation, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mAnimationView = view.findViewById(R.id.brave_1p_storage_cleaner_animation_view);

        if (mAnimationView == null) {
            return;
        }

        mBlackFadeOverlay = view.findViewById(R.id.black_fade_overlay);

        mAnimationView.setAnimation(ANIMATION_FILE);
        mAnimationView.setRepeatCount(ANIMATION_REPEAT_COUNT);
        mAnimationView.addAnimatorListener(
                new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animation) {}

                    @Override
                    public void onAnimationEnd(Animator animation) {
                        hideAnimationAndStartFadeOut();
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {}

                    @Override
                    public void onAnimationRepeat(Animator animation) {}
                });

        // Start animation
        mAnimationView.playAnimation();
    }

    private void hideAnimationAndStartFadeOut() {
        if (mAnimationView != null) {
            // Hide the animation view since it's finished
            mAnimationView.setVisibility(View.GONE);
        }
        if (mBlackFadeOverlay != null) {
            mBlackFadeOverlay.setVisibility(View.VISIBLE);
            mBlackFadeOverlay
                    .animate()
                    .alpha(0.0f)
                    .setDuration(FADE_OUT_DURATION_MS)
                    .setStartDelay(FADE_OUT_START_DELAY_MS)
                    .setListener(
                            new Animator.AnimatorListener() {
                                @Override
                                public void onAnimationStart(Animator animation) {}

                                @Override
                                public void onAnimationEnd(Animator animation) {
                                    // Close the dialog fragment
                                    dismiss();
                                }

                                @Override
                                public void onAnimationCancel(Animator animation) {}

                                @Override
                                public void onAnimationRepeat(Animator animation) {}
                            })
                    .start();
        }
    }
}
