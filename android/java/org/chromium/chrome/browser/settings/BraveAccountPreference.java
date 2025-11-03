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
    private final int mTitleTextColor;
    private final int mSummaryTextColor;

    public BraveAccountPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.brave_account_preference);
        mTitleTextColor = a.getColor(R.styleable.brave_account_preference_title_text_color, 0);
        mSummaryTextColor = a.getColor(R.styleable.brave_account_preference_summary_text_color, 0);
        a.recycle();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (holder.findViewById(android.R.id.title) instanceof TextView titleView) {
            if (mTitleTextColor != 0) {
                titleView.setTextColor(mTitleTextColor);
            } else if (!isSelectable()) {
                // Restore the default primary text color when the preference is non-selectable.
                TypedArray ta =
                        getContext()
                                .obtainStyledAttributes(
                                        new int[] {android.R.attr.textColorPrimary});
                int defaultTitleColor = ta.getColor(0, titleView.getCurrentTextColor());
                ta.recycle();
                titleView.setTextColor(defaultTitleColor);
            }
        }

        if (mSummaryTextColor != 0
                && holder.findViewById(android.R.id.summary) instanceof TextView summaryView) {
            summaryView.setTextColor(mSummaryTextColor);
        }
    }
}
