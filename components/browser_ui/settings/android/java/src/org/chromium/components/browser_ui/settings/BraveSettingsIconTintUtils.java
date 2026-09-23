/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.settings;

import android.content.Context;
import android.content.res.ColorStateList;
import android.view.View;
import android.widget.ImageView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;

@NullMarked
public final class BraveSettingsIconTintUtils {
    private static final int[] DISABLED_STATE_SET = new int[] {-android.R.attr.state_enabled};
    private static final int[] DEFAULT_STATE_SET = new int[] {};
    private static final int DEFAULT_ICON_COLOR_TINT_LIST =
            org.chromium.components.browser_ui.styles.R.color.default_icon_color_tint_list;

    private BraveSettingsIconTintUtils() {}

    /**
     * Applies an icon tint to the standard preference icon in {@code holder}.
     *
     * <p>When {@code dynamicColorsEnabled} is true, the enabled-state color uses the current
     * Dynamic Colors accent. Otherwise, this applies the default resource tint.
     */
    public static void applyIconTint(PreferenceViewHolder holder, boolean dynamicColorsEnabled) {
        applyIconTint(holder.findViewById(android.R.id.icon), dynamicColorsEnabled);
    }

    /** Clears an icon tint from the standard preference icon in {@code holder}. */
    public static void clearIconTint(PreferenceViewHolder holder) {
        clearIconTint(holder.findViewById(android.R.id.icon));
    }

    /**
     * Applies an icon tint to {@code view} when it is an {@link ImageView}.
     *
     * <p>When {@code dynamicColorsEnabled} is true, the enabled-state color uses the current
     * Dynamic Colors accent. Otherwise, this applies the default resource tint. Non-image views are
     * unchanged.
     */
    public static void applyIconTint(@Nullable View view, boolean dynamicColorsEnabled) {
        if (view instanceof ImageView icon) {
            Context context = icon.getContext();
            ColorStateList defaultIconTint =
                    context.getColorStateList(DEFAULT_ICON_COLOR_TINT_LIST);
            ColorStateList iconTint =
                    dynamicColorsEnabled
                            ? createDynamicColorsIconTint(context, defaultIconTint)
                            : defaultIconTint;
            icon.setImageTintList(iconTint);
        }
    }

    /** Clears an icon tint from {@code view} when it is an {@link ImageView}. */
    public static void clearIconTint(@Nullable View view) {
        if (view instanceof ImageView icon) {
            icon.setImageTintList(null);
            icon.clearColorFilter();
        }
    }

    private static ColorStateList createDynamicColorsIconTint(
            Context context, ColorStateList defaultIconTint) {
        int disabledColor =
                defaultIconTint.getColorForState(
                        DISABLED_STATE_SET, defaultIconTint.getDefaultColor());
        int enabledColor = SemanticColorUtils.getDefaultIconColorAccent1(context);

        return new ColorStateList(
                new int[][] {DISABLED_STATE_SET, DEFAULT_STATE_SET},
                new int[] {disabledColor, enabledColor});
    }
}
