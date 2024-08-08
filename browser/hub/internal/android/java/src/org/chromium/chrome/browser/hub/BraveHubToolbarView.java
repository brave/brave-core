/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import android.content.Context;
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;

/**
 * Brave's extension for {@link HubToolbarView}. Here we control what elements should be visible in
 * tab switcher mode when bottom toolbar is visible.
 */
public class BraveHubToolbarView extends HubToolbarView {
    private Button mActionButton;
    private FrameLayout mMenuButton;

    public BraveHubToolbarView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mActionButton = findViewById(R.id.toolbar_action_button);
        mMenuButton = findViewById(R.id.menu_button_wrapper);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        updateButtonsVisibility();
    }

    @Override
    void setMenuButtonVisible(boolean visible) {
        super.setMenuButtonVisible(visible);

        updateButtonsVisibility();
    }

    @Override
    void setActionButton(@Nullable FullButtonData buttonData, boolean showText) {
        super.setActionButton(buttonData, showText);

        updateButtonsVisibility();
    }

    private void updateButtonsVisibility() {
        boolean isMenuFromBottom =
                ContextUtils.getAppSharedPreferences()
                        .getBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);

        mActionButton.setVisibility(isMenuFromBottom ? View.INVISIBLE : View.VISIBLE);
        mMenuButton.setVisibility(isMenuFromBottom ? View.INVISIBLE : View.VISIBLE);
    }
}
