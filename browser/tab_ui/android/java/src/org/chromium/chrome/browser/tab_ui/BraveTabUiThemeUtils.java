/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_ui;

import android.content.Context;

import androidx.annotation.ColorInt;

public class BraveTabUiThemeUtils {
    @ColorInt
    public static int getTitleTextColor(Context context, boolean isIncognito, boolean isSelected) {
        // These checks are just making sure that these values are still used in Chromium to avoid
        // lint issues.
        assert R.color.empty_state_icon_bg_background_color > 0
                : "Something has changed in the upstream!";
        assert R.color.empty_state_icon_bg_foreground_color > 0
                : "Something has changed in the upstream!";
        assert R.color.empty_state_icon_color > 0 : "Something has changed in the upstream!";
        assert R.color.empty_state_icon_bg_color > 0 : "Something has changed in the upstream!";
        assert R.color.empty_state_icon_tabswitcher_bg_color > 0
                : "Something has changed in the upstream!";

        if (isSelected) {
            return context.getColor(R.color.baseline_neutral_10);
        }

        return TabUiThemeUtils.getTitleTextColor(context, isIncognito, isSelected);
    }

    @ColorInt
    public static int getCardViewBackgroundColor(
            Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected && !isIncognito) {
            return context.getColor(R.color.baseline_primary_80);
        }

        return TabUiThemeUtils.getCardViewBackgroundColor(context, isIncognito, isSelected);
    }
}
