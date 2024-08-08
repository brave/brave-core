/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.menu_button;

import android.app.Activity;
import android.graphics.Canvas;
import android.view.View;

import androidx.annotation.IdRes;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

public class BraveMenuButtonCoordinator extends MenuButtonCoordinator {
    private Activity mActivity;

    public BraveMenuButtonCoordinator(
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            BrowserStateBrowserControlsVisibilityDelegate controlsVisibilityDelegate,
            WindowAndroid windowAndroid, SetFocusFunction setUrlBarFocusFunction,
            Runnable requestRenderRunnable, boolean shouldShowAppUpdateBadge,
            Supplier<Boolean> isInOverviewModeSupplier, ThemeColorProvider themeColorProvider,
            Supplier<MenuButtonState> menuButtonStateSupplier, Runnable onMenuButtonClicked,
            @IdRes int menuButtonId) {
        super(appMenuCoordinatorSupplier, controlsVisibilityDelegate, windowAndroid,
                setUrlBarFocusFunction, requestRenderRunnable, shouldShowAppUpdateBadge,
                isInOverviewModeSupplier, themeColorProvider, menuButtonStateSupplier,
                onMenuButtonClicked, menuButtonId);

        mActivity = windowAndroid.getActivity().get();
    }

    @Override
    public MenuButton getMenuButton() {
        updateMenuButtonState();
        return isMenuFromBottom() ? null : super.getMenuButton();
    }

    @Override
    public void drawTabSwitcherAnimationOverlay(View root, Canvas canvas, int alpha) {
        if (isMenuFromBottom()) return;
        super.drawTabSwitcherAnimationOverlay(root, canvas, alpha);
    }

    @Override
    public void setVisibility(boolean visible) {
        updateMenuButtonState();
        super.setVisibility(isMenuFromBottom() ? false : visible);
    }

    private void updateMenuButtonState() {
        BraveToolbarLayoutImpl layout =
                (BraveToolbarLayoutImpl) mActivity.findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.updateMenuButtonState();
        }
    }

    public static void setMenuFromBottom(boolean isMenuFromBottom) {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, isMenuFromBottom)
                .apply();
    }

    public static boolean isMenuFromBottom() {
        return ContextUtils.getAppSharedPreferences()
                .getBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);
    }

    public static void setupPropertyModel(
            MenuButton menuButton, Supplier<MenuButtonState> menuButtonStateSupplier) {
        PropertyModel menuButtonPropertyModel =
                new PropertyModel.Builder(MenuButtonProperties.ALL_KEYS)
                        .with(MenuButtonProperties.STATE_SUPPLIER, menuButtonStateSupplier)
                        .build();
        PropertyModelChangeProcessor.create(
                menuButtonPropertyModel, menuButton, new MenuButtonViewBinder());
    }
}
