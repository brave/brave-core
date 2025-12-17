/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_shields;

import android.animation.Animator;
import android.app.Dialog;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.content.Context;

import org.chromium.build.annotations.NullMarked;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentActivity;

import com.airbnb.lottie.LottieAnimationView;

@NullMarked
public class FirstPartyStorageCleanerAnimationFragment extends DialogFragment {
    private static final String TAG = "FPStorageCleanerAnimFrag";
    private static final String ANIMATION_FILE = "brave_first_party_storage_clean_animation.json";

    private LottieAnimationView mAnimationView;
    private OnAnimationCompleteListener mAnimationCompleteListener;

    public interface OnAnimationCompleteListener {
        void onAnimationComplete();
    }

    public static void show(Context context) {
        FragmentManager fragmentManager =
                ((FragmentActivity) context).getSupportFragmentManager();
        FirstPartyStorageCleanerAnimationFragment fragment =
                new FirstPartyStorageCleanerAnimationFragment();
        fragment.show(fragmentManager, TAG);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Use full-screen dialog style
        setStyle(DialogFragment.STYLE_NO_TITLE, android.R.style.Theme_Black_NoTitleBar_Fullscreen);
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);

        // Make dialog full-screen with transparent background
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
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(
                R.layout.brave_first_party_storage_cleaner_animation, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mAnimationView = view.findViewById(R.id.brave_1p_storage_cleaner_animation_view);
        mAnimationView.setAnimation(ANIMATION_FILE);

        // Configure animation properties
        mAnimationView.setRepeatCount(0);

        // Add animation listener
        mAnimationView.addAnimatorListener(
                new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animation) {
                        // Animation started
                    }

                    @Override
                    public void onAnimationEnd(Animator animation) {
                        // Auto-dismiss after animation completes
                        dismiss();
                        if (mAnimationCompleteListener != null) {
                            mAnimationCompleteListener.onAnimationComplete();
                        }
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {
                        // Animation cancelled
                    }

                    @Override
                    public void onAnimationRepeat(Animator animation) {
                        // Animation repeated (for looping animations)
                    }
                });

        // Allow clicking outside to dismiss (optional)
        view.setOnClickListener(
                v -> {
                    if (mAnimationView.isAnimating()) {
                        mAnimationView.cancelAnimation();
                    }
                    dismiss();
                });

        // Start animation
        if (mAnimationView != null) {
            mAnimationView.playAnimation();
        }
    }
}
