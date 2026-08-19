/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.theme.BraveDynamicColors;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.ui.text.SpanApplier;

/** A preference with an inline text-button title span. */
@NullMarked
public class BraveInlineTextButtonPreference extends ChromeBasePreference {
    private static final String TEXT_BUTTON_START_TAG = "<LINK_1>";
    private static final String TEXT_BUTTON_END_TAG = "</LINK_1>";

    private int mTextButtonStart = -1;
    private int mTextButtonEnd = -1;

    public BraveInlineTextButtonPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Sets a title whose {@code <LINK_1>...</LINK_1>} range is styled as a text button after
     * binding.
     *
     * <p>{@code title} must contain a valid {@code <LINK_1>...</LINK_1>} range. Missing or invalid
     * tags cause {@link IllegalArgumentException}.
     *
     * @param title title string with the required {@code <LINK_1>...</LINK_1>} markup
     * @throws IllegalArgumentException if {@code title} does not contain a valid {@code
     *     <LINK_1>...</LINK_1>} range
     */
    public void setTextButtonTitle(String title) {
        Object marker = new Object();
        SpannableString formattedTitle =
                SpanApplier.applySpans(
                        title,
                        new SpanApplier.SpanInfo(
                                TEXT_BUTTON_START_TAG, TEXT_BUTTON_END_TAG, marker));
        mTextButtonStart = formattedTitle.getSpanStart(marker);
        mTextButtonEnd = formattedTitle.getSpanEnd(marker);
        formattedTitle.removeSpan(marker);
        setTitle(formattedTitle);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (mTextButtonStart < 0
                || !(holder.findViewById(android.R.id.title) instanceof TextView titleView)
                || mTextButtonEnd > titleView.length()) {
            return;
        }

        SpannableString styledTitle = new SpannableString(titleView.getText());
        styledTitle.setSpan(
                new ForegroundColorSpan(
                        BraveDynamicColors.getTextButtonColor(
                                getContext().getTheme(),
                                getContext().getColor(R.color.brave_link))),
                mTextButtonStart,
                mTextButtonEnd,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        titleView.setText(styledTitle);
    }
}
