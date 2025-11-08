/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;

/**
 * A preference for Brave Account that supports customizing title and summary text colors
 * and making the title bold.
 */
@NullMarked
public class BraveAccountPreference extends ChromeBasePreference {
    private final int mTitleTextColor;
    private final int mSummaryTextColor;
    private final boolean mBoldTitle;

    /** Constructor for BraveAccountPreference. */
    public BraveAccountPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.BraveAccountPreference);
        mTitleTextColor = a.getColor(
                R.styleable.BraveAccountPreference_titleTextColor,
                /* defaultValue= */ 0);
        mSummaryTextColor = a.getColor(
                R.styleable.BraveAccountPreference_summaryTextColor,
                /* defaultValue= */ 0);
        mBoldTitle = a.getBoolean(R.styleable.BraveAccountPreference_boldTitle, false);
        a.recycle();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        // Customize title
        if (holder.findViewById(android.R.id.title) instanceof TextView titleView) {
            if (mTitleTextColor != 0) {
                titleView.setTextColor(mTitleTextColor);
            }
            if (mBoldTitle) {
                titleView.setTypeface(titleView.getTypeface(), Typeface.BOLD);
            }
        }

        // Customize summary
        if (mSummaryTextColor != 0
                && holder.findViewById(android.R.id.summary) instanceof TextView summaryView) {
            summaryView.setTextColor(mSummaryTextColor);
        }
    }
}
