/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.ui.text.NoUnderlineClickableSpan;

public class AndroidUtils {
    public static int getToolBarHeight(Context context) {
        TypedValue tv = new TypedValue();
        if (context.getTheme().resolveAttribute(android.R.attr.actionBarSize, tv, true)) {
            return TypedValue.complexToDimensionPixelSize(
                    tv.data, context.getResources().getDisplayMetrics());
        }
        return 0;
    }

    public static Spanned formatHTML(String html) {
        return Html.fromHtml(html, Html.FROM_HTML_MODE_LEGACY);
    }

    // Views
    public static TextView makeHeaderTv(Context context) {
        TextView textView = new TextView(context);
        textView.setTextAppearance(R.style.BraveWalletTextViewTitle);
        textView.setTypeface(null, Typeface.BOLD);
        textView.setId(View.generateViewId());
        return textView;
    }

    public static TextView makeSubHeaderTv(Context context) {
        TextView textView = new TextView(context);
        textView.setTextAppearance(R.style.BraveWalletTextViewSubTitle);
        textView.setId(View.generateViewId());
        return textView;
    }

    public static void disable(@NonNull View... views) {
        for (View view : views) {
            view.setEnabled(false);
        }
    }

    public static void gone(View... views) {
        setViewVisibility(false, views);
    }

    public static void show(View... views) {
        setViewVisibility(true, views);
    }

    public static void invisible(View... views) {
        for (View view : views) {
            view.setVisibility(View.INVISIBLE);
        }
    }

    private static void setViewVisibility(boolean isVisible, View... views) {
        int visibility = isVisible ? View.VISIBLE : View.GONE;
        for (View view : views) {
            view.setVisibility(visibility);
        }
    }

    /**
     * Gets device screen height in pixels, excluding the navigation bar (if visible) and insets.
     *
     * @return device screen height in pixels.
     */
    public static int getScreenHeight() {
        return Resources.getSystem().getDisplayMetrics().heightPixels;
    }

    /**
     * Check if the fragment is safe to update its UI
     *
     * @param frag instance
     * @return true if Fragment UI can be updated otherwise false
     */
    public static boolean canUpdateFragmentUi(Fragment frag) {
        return !(frag.isRemoving()
                || frag.getActivity() == null
                || frag.isDetached()
                || !frag.isAdded()
                || frag.getView() == null);
    }

    /**
     * Calculated an ideal row count for shimmer effect based on screen size
     *
     * @param skeletonRowHeight of a skeleton row view in pixels
     * @return count of rows for the skeleton list
     */
    public static int getSkeletonRowCount(int skeletonRowHeight) {
        int pxHeight = getScreenHeight();
        return (int) Math.floor(pxHeight / skeletonRowHeight);
    }

    /**
     * @return {@code true} if the app is a debug build, {@code false} otherwise.
     */
    public static boolean isDebugBuild() {
        return (ContextUtils.getApplicationContext().getApplicationInfo().flags
                        & ApplicationInfo.FLAG_DEBUGGABLE)
                != 0;
    }

    public static SpannableString createClickableSpanString(
            Context context, @StringRes int id, Callback listener) {
        NoUnderlineClickableSpan noUnderlineClickableSpan =
                new NoUnderlineClickableSpan(context, R.color.brave_link, listener);
        SpannableString spannableString = new SpannableString(context.getString(id));
        spannableString.setSpan(noUnderlineClickableSpan, 0, spannableString.length(), 0);
        return spannableString;
    }
}
