/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.brave.browser.custom_app_icons.CustomAppIconsManager;
import org.chromium.brave.browser.custom_app_icons.R;

public class CustomAppIconsPreference extends Preference {

    public CustomAppIconsPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.custom_app_icons_item);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        // Find the icon view
        ImageView iconView = (ImageView) holder.findViewById(R.id.icon);
        if (iconView != null) {
            iconView.setVisibility(View.GONE);
        }

        ImageView icon24View = (ImageView) holder.findViewById(R.id.icon_24);
        if (icon24View != null) {
            int iconDrawable = CustomAppIconsManager.getCurrentIcon(getContext()).getIcon();
            icon24View.setImageResource(iconDrawable);
            icon24View.setVisibility(View.VISIBLE);
        }

        TextView titleView = (TextView) holder.findViewById(R.id.title);
        if (titleView != null) {
            titleView.setText(getContext().getString(R.string.change_app_icons));
        }

        View divider = holder.findViewById(R.id.divider);
        if (divider != null) {
            divider.setVisibility(View.GONE);
        }
    }
}
