/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.text.Html;
import android.text.Spanned;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.components.browser_ui.settings.TextMessagePreference;

/** A preference that displays the Brave Origin description text. */
@NullMarked
public class BraveOriginDescriptionPreference extends TextMessagePreference {
    public BraveOriginDescriptionPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        TextView summaryView = (TextView) holder.findViewById(android.R.id.summary);
        if (summaryView != null) {
            // The text is too long, so we need to set the max lines to the maximum value
            // otherwise it will be cut off
            summaryView.setMaxLines(Integer.MAX_VALUE);

            // Parse HTML to support bold text
            CharSequence summary = getSummary();
            if (summary != null) {
                Spanned styledText = Html.fromHtml(summary.toString(), Html.FROM_HTML_MODE_LEGACY);
                summaryView.setText(styledText);
            }
        }
    }
}
