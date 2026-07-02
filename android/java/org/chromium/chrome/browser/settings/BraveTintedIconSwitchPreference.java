/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assertNonNull;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;

import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.widget.ImageViewCompat;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** A Brave switch preference that tints its icon for enabled and disabled states. */
@NullMarked
public class BraveTintedIconSwitchPreference extends ChromeSwitchPreference {
    public BraveTintedIconSwitchPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (holder.findViewById(android.R.id.icon) instanceof ImageView icon) {
            ImageViewCompat.setImageTintList(
                    icon,
                    assertNonNull(
                            AppCompatResources.getColorStateList(
                                    getContext(), R.color.default_icon_color_tint_list)));
        }
    }
}
