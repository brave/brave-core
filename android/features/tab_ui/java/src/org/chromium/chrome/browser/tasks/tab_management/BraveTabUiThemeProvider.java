/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;
import android.content.res.ColorStateList;

import androidx.annotation.ColorInt;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.tab_ui.R;

public class BraveTabUiThemeProvider {
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

        return TabUiThemeProvider.getTitleTextColor(context, isIncognito, isSelected);
    }

    public static ColorStateList getActionButtonTintList(
            Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected) {
            return AppCompatResources.getColorStateList(context, R.color.baseline_neutral_10);
        }

        return TabUiThemeProvider.getActionButtonTintList(context, isIncognito, isSelected);
    }

    @ColorInt
    public static int getCardViewBackgroundColor(
            Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected && !isIncognito) {
            return context.getColor(R.color.brave_tab_view_card_selected_bg);
        }

        return TabUiThemeProvider.getCardViewBackgroundColor(context, isIncognito, isSelected);
    }
}
