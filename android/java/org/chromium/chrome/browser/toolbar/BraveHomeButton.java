/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.TintObserver;

/**
 * Brave's extension of HomeButton.
 */
public class BraveHomeButton extends HomeButton implements TintObserver {
    private ThemeColorProvider mThemeColorProvider;

    public BraveHomeButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        // This check is just making sure that this dimen is still used in Chromium to avoid lint
        // issues.
        assert R.dimen.home_button_list_menu_width > 0 : "Something has changed in the upstream!";
    }

    public void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addTintObserver(this);
    }

    @Override
    public void onTintChanged(ColorStateList tint, int brandedColorScheme) {
        ApiCompatibilityUtils.setImageTintList(this, tint);
    }

    public void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }
    }
}
