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
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

public class BraveMenuButtonCoordinator extends MenuButtonCoordinator {
    private final Activity mActivity;

    public BraveMenuButtonCoordinator(
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            BrowserStateBrowserControlsVisibilityDelegate controlsVisibilityDelegate,
            WindowAndroid windowAndroid,
            SetFocusFunction setUrlBarFocusFunction,
            Runnable requestRenderRunnable,
            boolean canShowAppUpdateBadge,
            Supplier<Boolean> isInOverviewModeSupplier,
            ThemeColorProvider themeColorProvider,
            Supplier<MenuButtonState> menuButtonStateSupplier,
            Runnable onMenuButtonClicked,
            @IdRes int menuButtonId,
            @Nullable VisibilityDelegate visibilityDelegate) {
        super(
                appMenuCoordinatorSupplier,
                controlsVisibilityDelegate,
                windowAndroid,
                setUrlBarFocusFunction,
                requestRenderRunnable,
                canShowAppUpdateBadge,
                isInOverviewModeSupplier,
                themeColorProvider,
                menuButtonStateSupplier,
                onMenuButtonClicked,
                menuButtonId,
                visibilityDelegate);

        mActivity = windowAndroid.getActivity().get();
    }

    @Override
    public MenuButton getMenuButton() {
        updateMenuButtonState();
        return BottomToolbarConfiguration.isToolbarTopAnchored() && isMenuFromBottom()
                ? null
                : super.getMenuButton();
    }

    @Override
    public void drawTabSwitcherAnimationOverlay(View root, Canvas canvas, int alpha) {
        if (BottomToolbarConfiguration.isToolbarTopAnchored() && isMenuFromBottom()) return;
        super.drawTabSwitcherAnimationOverlay(root, canvas, alpha);
    }

    @Override
    public void setVisibility(boolean visible) {
        updateMenuButtonState();

        // Remove menu from top address bar if it is shown in the bottom controls.
        super.setVisibility(
                (isMenuFromBottom() && BottomToolbarConfiguration.isToolbarTopAnchored())
                        ? false
                        : visible);
    }

    private void updateMenuButtonState() {
        BraveToolbarLayoutImpl layout =
                (BraveToolbarLayoutImpl) mActivity.findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.updateMenuButtonState();
        }
    }

    public boolean isToolbarBottomAnchored() {
        return BottomToolbarConfiguration.isToolbarBottomAnchored();
    }

    public static void setMenuFromBottom(boolean isMenuFromBottom) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, isMenuFromBottom);
    }

    public static boolean isMenuFromBottom() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);
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
