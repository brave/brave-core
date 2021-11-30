/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;
import android.content.res.ColorStateList;

import androidx.annotation.ColorInt;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.tab_ui.R;

public class BraveTabUiThemeProvider {
    @ColorInt
    public static int getTitleTextColor(Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected) {
            return ApiCompatibilityUtils.getColor(
                    context.getResources(), R.color.baseline_neutral_900);
        }

        return TabUiThemeProvider.getTitleTextColor(context, isIncognito, isSelected);
    }

    public static ColorStateList getActionButtonTintList(
            Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected) {
            return AppCompatResources.getColorStateList(context, R.color.baseline_neutral_900);
        }

        return TabUiThemeProvider.getActionButtonTintList(context, isIncognito, isSelected);
    }
}
