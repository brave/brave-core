/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;
import android.content.res.ColorStateList;

import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.tab_ui.R;

public class BraveTabUiThemeProvider {
    public static ColorStateList getActionButtonTintList(
            Context context, boolean isIncognito, boolean isSelected) {
        if (isSelected) {
            return AppCompatResources.getColorStateList(context, R.color.baseline_neutral_10);
        }

        return TabUiThemeProvider.getActionButtonTintList(context, isIncognito, isSelected);
    }
}
