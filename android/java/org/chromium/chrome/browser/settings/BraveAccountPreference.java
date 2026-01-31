/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;

/** A preference for Brave Account that supports customizing title and summary text colors. */
@NullMarked
public class BraveAccountPreference extends ChromeBasePreference {
    private final int mTitleTextColorResId;
    private final int mSummaryTextColorResId;

    public BraveAccountPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.brave_account_preference);
        mTitleTextColorResId =
                a.getResourceId(R.styleable.brave_account_preference_title_text_color, 0);
        mSummaryTextColorResId =
                a.getResourceId(R.styleable.brave_account_preference_summary_text_color, 0);
        a.recycle();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (holder.findViewById(android.R.id.title) instanceof TextView titleView) {
            if (!isSelectable()) {
                // Restore the default primary text color when the preference is non-selectable.
                setColorFromAttr(titleView, android.R.attr.textColorPrimary);
            } else if (!isEnabled()) {
                // Use tertiary text color when preference is disabled.
                setColorFromRes(titleView, R.color.text_tertiary);
            } else {
                setColorFromRes(titleView, mTitleTextColorResId);
            }
        }

        if (holder.findViewById(android.R.id.summary) instanceof TextView summaryView) {
            setColorFromRes(summaryView, mSummaryTextColorResId);
        }
    }

    private void setColorFromAttr(TextView textView, int attr) {
        TypedArray ta = getContext().obtainStyledAttributes(new int[] {attr});
        int color = ta.getColor(0, textView.getCurrentTextColor());
        ta.recycle();
        textView.setTextColor(color);
    }

    private void setColorFromRes(TextView textView, int colorResId) {
        if (colorResId != 0) {
            textView.setTextColor(getContext().getColor(colorResId));
        }
    }
}
