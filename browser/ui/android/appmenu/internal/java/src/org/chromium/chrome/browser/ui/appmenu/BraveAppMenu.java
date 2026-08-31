/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.annotation.SuppressLint;
import android.content.res.Resources;
import android.graphics.Rect;
import android.view.View;
import android.widget.PopupWindow;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.SysUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.ui.appmenu.internal.R;
import org.chromium.ui.hierarchicalmenu.HierarchicalMenuController;

@NullMarked
public class BraveAppMenu extends BraveAppMenuDummySuper {
    private static final int BOTTOM_MENU_VERTICAL_OFFSET_DP = 44;
    private static int sMenuHeight;
    private static int sNegativeVerticalOffsetNotTopAnchored;

    BraveAppMenu(
            AppMenuVisibilityDelegate visibilityDelegate,
            Resources res,
            HierarchicalMenuController<AppMenu.AppMenuPopup> hierarchicalMenuController,
            boolean disableVerticalScrollbar) {
        super(visibilityDelegate, res, hierarchicalMenuController, disableVerticalScrollbar);

        final float scale = res.getDisplayMetrics().density;
        sNegativeVerticalOffsetNotTopAnchored =
                (int) (BOTTOM_MENU_VERTICAL_OFFSET_DP * scale + 0.5f);
    }

    public static Class getAppMenuVisibilityDelegateClass() {
        return AppMenuVisibilityDelegate.class;
    }

    @SuppressLint("VisibleForTests")
    public static int[] getPopupPosition(
            int[] tempLocation,
            boolean isByPermanentButton,
            boolean isFromBottomBar,
            int negativeSoftwareVerticalOffset,
            int screenRotation,
            Rect appRect,
            Rect padding,
            View anchorView,
            int popupWidth,
            int popupHeight,
            int viewLayoutDirection,
            boolean positionBelowAnchor) {
        int[] position =
                AppMenu.getPopupPosition(
                        tempLocation,
                        isByPermanentButton,
                        isFromBottomBar,
                        negativeSoftwareVerticalOffset,
                        screenRotation,
                        appRect,
                        padding,
                        anchorView,
                        popupWidth,
                        popupHeight,
                        viewLayoutDirection,
                        positionBelowAnchor);
        if (isMenuFromBottom()) {
            anchorView.getLocationOnScreen(tempLocation);
            int anchorViewLocationOnScreenY = tempLocation[1];
            position[1] += appRect.bottom - anchorViewLocationOnScreenY - sMenuHeight;
            position[1] -= sNegativeVerticalOffsetNotTopAnchored;
            position[1] += padding.bottom;
        }
        return position;
    }

    // We shouldn't determine menu position by reading preference.
    // Ideally we should add this method to AppMenuHandler interface.
    @SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
    public static boolean isMenuFromBottom() {
        return ContextUtils.getAppSharedPreferences()
                .getBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, false);
    }

    @Override
    public void runMenuItemEnterAnimations() {
        // We do nothing here as we don't want any fancy animation for the menu.
    }

    public void updatePopup(PopupWindow popup, boolean isByPermanentButton) {
        // Upstream already animates the menu up from the bottom when it is anchored to the bottom
        // bar or to a bottom anchored address bar, so only Brave's own bottom toolbar is left to
        // account for here. Overriding the animation unconditionally would undo those.
        if (!SysUtils.isLowEndDevice() && !isByPermanentButton && isMenuFromBottom()) {
            popup.setAnimationStyle(R.style.EndIconMenuAnimBottom);
        }
        sMenuHeight = popup.getHeight();
    }
}
