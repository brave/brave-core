/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.menu_button;

import android.app.Activity;
import android.content.SharedPreferences;

import androidx.annotation.IdRes;

import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.toolbar.ThemeColorProvider;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;

public class BraveMenuButtonCoordinator extends MenuButtonCoordinator {
    private static final String BRAVE_IS_MENU_FROM_BOTTOM = "brave_is_menu_from_bottom";

    public BraveMenuButtonCoordinator(
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            BrowserStateBrowserControlsVisibilityDelegate controlsVisibilityDelegate,
            Activity activity, SetFocusFunction setUrlBarFocusFunction,
            Runnable requestRenderRunnable, boolean shouldShowAppUpdateBadge,
            Supplier<Boolean> isInOverviewModeSupplier, ThemeColorProvider themeColorProvider,
            @IdRes int menuButtonId) {
        super(appMenuCoordinatorSupplier, controlsVisibilityDelegate, activity,
                setUrlBarFocusFunction, requestRenderRunnable, shouldShowAppUpdateBadge,
                isInOverviewModeSupplier, themeColorProvider, menuButtonId);
    }

    @Override
    public MenuButton getMenuButton() {
        return isMenuFromBottom() ? null : super.getMenuButton();
    }

    @Override
    public void setVisibility(boolean visible) {
        super.setVisibility(isMenuFromBottom() ? false : visible);
    }

    public static void setMenuFromBottom(boolean isMenuFromBottom) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        prefs.edit().putBoolean(BRAVE_IS_MENU_FROM_BOTTOM, isMenuFromBottom).apply();
    }

    public static boolean isMenuFromBottom() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BRAVE_IS_MENU_FROM_BOTTOM, false);
    }
}
