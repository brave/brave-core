/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_ui;

import android.content.Context;
import android.content.res.ColorStateList;

import androidx.annotation.ColorInt;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.components.tab_groups.TabGroupColorId;

@NullMarked
public class BraveTabCardThemeUtil {
    @ColorInt
    public static int getTitleTextColor(
            Context context,
            boolean isIncognito,
            boolean isSelected,
            @Nullable @TabGroupColorId Integer colorId) {
        if (isSelected) {
            return context.getColor(R.color.baseline_neutral_10);
        }

        return TabCardThemeUtil.getTitleTextColor(context, isIncognito, isSelected, colorId);
    }

    @ColorInt
    public static int getCardViewBackgroundColor(
            Context context,
            boolean isIncognito,
            boolean isSelected,
            @Nullable @TabGroupColorId Integer colorId) {
        if (isSelected && !isIncognito) {
            return context.getColor(R.color.baseline_primary_80);
        }

        return TabCardThemeUtil.getCardViewBackgroundColor(
                context, isIncognito, isSelected, colorId);
    }

    public static ColorStateList getActionButtonTintList(
            Context context,
            boolean isIncognito,
            boolean isSelected,
            @Nullable @TabGroupColorId Integer colorId) {
        if (isSelected) {
            return AppCompatResources.getColorStateList(context, R.color.baseline_neutral_10);
        }

        return TabCardThemeUtil.getActionButtonTintList(context, isIncognito, isSelected, colorId);
    }
}
