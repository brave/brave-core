/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.activity.ComponentActivity;
import androidx.annotation.VisibleForTesting;
import androidx.core.app.PictureInPictureModeChangedInfo;
import androidx.core.graphics.Insets;
import androidx.core.util.Consumer;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.chromium.base.ApplicationStatus;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;

/**
 * Shows the exclusive access notice (fullscreen, keyboard lock, pointer lock) as a small pill at
 * the top center of the window. Unlike the upstream bottom snackbar, it does not cover subtitles or
 * video controls, always hides after {@link #AUTO_HIDE_DELAY_MS}, shows again whenever the window
 * regains focus, and is never shown in picture-in-picture.
 *
 * <p>The notice is native Android UI above the web content, so a page cannot spoof or suppress it.
 */
@NullMarked
public class BraveExclusiveAccessNotice implements ApplicationStatus.WindowFocusChangedListener {
    @VisibleForTesting static final long AUTO_HIDE_DELAY_MS = 4000;

    private static final int MARGIN_DP = 16;
    private static final int PADDING_HORIZONTAL_DP = 20;
    private static final int PADDING_VERTICAL_DP = 12;
    private static final int CORNER_RADIUS_DP = 16;
    private static final int MAX_WIDTH_DP = 480;
    private static final int MAX_LINES = 4;
    private static final int BACKGROUND_COLOR = Color.argb(0xD9, 0x1C, 0x1C, 0x1E);

    private final ExclusiveAccessContext mContext;
    private final Handler mHandler = new Handler(Looper.getMainLooper());
    private final Runnable mAutoHide = this::removeView;
    private final Consumer<PictureInPictureModeChangedInfo> mPictureInPictureListener =
            info -> onPictureInPictureModeChanged(info.isInPictureInPictureMode());

    private final View.OnLayoutChangeListener mLayoutChangeListener =
            (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) ->
                    updateTopMargin();

    private @Nullable String mText;
    private @Nullable TextView mView;
    private @Nullable ViewGroup mContainer;
    private @Nullable ComponentActivity mObservedActivity;

    BraveExclusiveAccessNotice(ExclusiveAccessContext context) {
        mContext = context;
    }

    /**
     * Shows {@code text}, or the last text when null.
     *
     * @return Whether this notice handles display; false means the caller should fall back to the
     *     upstream snackbar.
     */
    boolean show(@Nullable String text) {
        if (text != null) mText = text;
        if (mText == null) return true;
        if (getContentContainer() == null) return false;
        startObserving();
        display();
        return true;
    }

    /**
     * Updates the text and shows it, unless the same text is already visible, in which case the
     * hide timer keeps running.
     *
     * @return Whether this notice handles display; false means the caller should fall back to the
     *     upstream snackbar.
     */
    boolean update(String text) {
        if (text.equals(mText) && isVisible()) return true;
        return show(text);
    }

    /** Hides the notice and ends the session; it is no longer re-shown on focus changes. */
    void hide() {
        stopObserving();
        removeView();
        mText = null;
    }

    boolean isVisible() {
        return mView != null;
    }

    @Override
    public void onWindowFocusChanged(Activity activity, boolean hasFocus) {
        // A notice that is still showing keeps its timer, so focus changes during a rotation
        // do not extend it.
        if (!hasFocus || mText == null || activity != mContext.getActivity() || isVisible()) {
            return;
        }
        display();
    }

    @VisibleForTesting
    void onPictureInPictureModeChanged(boolean inPictureInPicture) {
        if (inPictureInPicture) {
            removeView();
        } else {
            display();
        }
    }

    @VisibleForTesting
    boolean isInPictureInPicture(Activity activity) {
        return activity.isInPictureInPictureMode();
    }

    private void display() {
        removeView();
        Activity activity = mContext.getActivity();
        ViewGroup container = getContentContainer();
        if (mText == null
                || activity == null
                || container == null
                || activity.isFinishing()
                || activity.isDestroyed()
                || isInPictureInPicture(activity)) {
            return;
        }

        TextView view = createView(activity, mText);
        FrameLayout.LayoutParams params =
                new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        int margin = dpToPx(activity, MARGIN_DP);
        params.setMargins(margin, getTopInset(container) + margin, margin, 0);
        container.addView(view, params);
        mView = view;
        mContainer = container;
        // Rotation and insets changes lay the container out again. Only the margin is refreshed;
        // the hide timer is not restarted.
        container.addOnLayoutChangeListener(mLayoutChangeListener);
        mHandler.postDelayed(mAutoHide, AUTO_HIDE_DELAY_MS);
    }

    private void removeView() {
        mHandler.removeCallbacks(mAutoHide);
        if (mContainer != null) {
            mContainer.removeOnLayoutChangeListener(mLayoutChangeListener);
            mContainer = null;
        }
        TextView view = mView;
        mView = null;
        if (view != null && view.getParent() instanceof ViewGroup parent) {
            parent.removeView(view);
        }
    }

    private void updateTopMargin() {
        TextView view = mView;
        ViewGroup container = mContainer;
        Activity activity = mContext.getActivity();
        if (view == null
                || container == null
                || activity == null
                || !(view.getLayoutParams() instanceof FrameLayout.LayoutParams params)) {
            return;
        }
        int topMargin = getTopInset(container) + dpToPx(activity, MARGIN_DP);
        if (params.topMargin != topMargin) {
            params.topMargin = topMargin;
            view.setLayoutParams(params);
        }
    }

    private static TextView createView(Activity activity, String text) {
        TextView view = new TextView(activity);
        view.setText(text);
        view.setTextAppearance(R.style.TextAppearance_TextMedium_Primary_Baseline_Light);
        view.setGravity(Gravity.CENTER);
        view.setMaxLines(MAX_LINES);
        view.setMaxWidth(dpToPx(activity, MAX_WIDTH_DP));
        view.setPadding(
                dpToPx(activity, PADDING_HORIZONTAL_DP),
                dpToPx(activity, PADDING_VERTICAL_DP),
                dpToPx(activity, PADDING_HORIZONTAL_DP),
                dpToPx(activity, PADDING_VERTICAL_DP));
        GradientDrawable background = new GradientDrawable();
        background.setColor(BACKGROUND_COLOR);
        background.setCornerRadius(dpToPx(activity, CORNER_RADIUS_DP));
        view.setBackground(background);
        view.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        return view;
    }

    private @Nullable ViewGroup getContentContainer() {
        Activity activity = mContext.getActivity();
        if (activity == null) return null;
        return activity.findViewById(android.R.id.content);
    }

    private static int getTopInset(View container) {
        WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(container);
        if (insets == null) return 0;
        Insets bars =
                insets.getInsetsIgnoringVisibility(
                        WindowInsetsCompat.Type.systemBars()
                                | WindowInsetsCompat.Type.displayCutout());
        return bars.top;
    }

    private static int dpToPx(Activity activity, int dp) {
        return Math.round(
                TypedValue.applyDimension(
                        TypedValue.COMPLEX_UNIT_DIP,
                        dp,
                        activity.getResources().getDisplayMetrics()));
    }

    private void startObserving() {
        if (mObservedActivity != null) return;
        ApplicationStatus.registerWindowFocusChangedListener(this);
        if (mContext.getActivity() instanceof ComponentActivity activity) {
            mObservedActivity = activity;
            activity.addOnPictureInPictureModeChangedListener(mPictureInPictureListener);
        }
    }

    private void stopObserving() {
        ApplicationStatus.unregisterWindowFocusChangedListener(this);
        if (mObservedActivity != null) {
            mObservedActivity.removeOnPictureInPictureModeChangedListener(
                    mPictureInPictureListener);
            mObservedActivity = null;
        }
    }
}
