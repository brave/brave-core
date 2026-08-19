/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;

/** A preference for Brave Account with a custom title appearance and truncation behavior. */
@NullMarked
public class BraveAccountPreference extends ChromeBasePreference {
    private final int mTitleTextAppearanceResId;
    private final boolean mTitleTruncateMiddle;

    public BraveAccountPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.brave_account_preference);
        mTitleTextAppearanceResId =
                a.getResourceId(R.styleable.brave_account_preference_title_text_appearance, 0);
        mTitleTruncateMiddle =
                a.getBoolean(R.styleable.brave_account_preference_title_truncate_middle, false);

        a.recycle();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (holder.findViewById(android.R.id.title) instanceof TextView titleView) {
            if (!isSelectable()) {
                // Restore the default primary title appearance when the preference is
                // non-selectable.
                titleView.setTextAppearance(R.style.TextAppearance_TextLarge_Primary);
            } else if (!isEnabled()) {
                // Use the tertiary title appearance when the preference is disabled.
                titleView.setTextAppearance(R.style.TextAppearance_Brave_PreferenceTitle_Tertiary);
            } else if (mTitleTextAppearanceResId != 0) {
                titleView.setTextAppearance(mTitleTextAppearanceResId);
            }

            if (mTitleTruncateMiddle) {
                titleView.setSingleLine(true);
                titleView.setEllipsize(TextUtils.TruncateAt.MIDDLE);
            }
        }
    }
}
