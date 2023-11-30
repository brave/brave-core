/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Rect;
import android.view.View;
import android.widget.PopupWindow;

import org.chromium.base.ContextUtils;
import org.chromium.base.SysUtils;
import org.chromium.chrome.browser.ui.appmenu.internal.R;

class BraveAppMenu extends AppMenu {
    private static final String BRAVE_IS_MENU_FROM_BOTTOM = "brave_is_menu_from_bottom";

    private static int sMenuHeight;
    private static int sNegativeVerticalOffsetNotTopAnchored;

    BraveAppMenu(int itemRowHeight, AppMenuHandlerImpl handler, Resources res) {
        super(itemRowHeight, handler, res);

        sNegativeVerticalOffsetNotTopAnchored =
                res.getDimensionPixelSize(R.dimen.menu_negative_vertical_offset_not_top_anchored);
    }

    @SuppressLint("VisibleForTests")
    public static int[] getPopupPosition(
            int[] tempLocation,
            boolean isByPermanentButton,
            int negativeSoftwareVerticalOffset,
            int screenRotation,
            Rect appRect,
            Rect padding,
            View anchorView,
            int popupWidth,
            int viewLayoutDirection) {
        int[] position =
                AppMenu.getPopupPosition(
                        tempLocation,
                        isByPermanentButton,
                        negativeSoftwareVerticalOffset,
                        screenRotation,
                        appRect,
                        padding,
                        anchorView,
                        popupWidth,
                        viewLayoutDirection);
        if (isMenuFromBottom()) {
            anchorView.getLocationOnScreen(tempLocation);
            int anchorViewLocationOnScreenY = tempLocation[1];
            position[1] += appRect.bottom - anchorViewLocationOnScreenY - sMenuHeight;
            position[1] -= sNegativeVerticalOffsetNotTopAnchored;
            position[1] += padding.bottom;
        }
        return position;
    }

    public static int getAnimationStyle() {
        return isMenuFromBottom() ? R.style.EndIconMenuAnimBottom : R.style.EndIconMenuAnim;
    }

    public static boolean isMenuFromBottom() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BRAVE_IS_MENU_FROM_BOTTOM, false);
    }

    public void runMenuItemEnterAnimations() {
        // We do nothing here as we don't want any fancy animation for the menu.
    }

    public void updatePopup(PopupWindow popup, boolean isByPermanentButton) {
        if (!SysUtils.isLowEndDevice() && !isByPermanentButton) {
            popup.setAnimationStyle(BraveAppMenu.getAnimationStyle());
        }
        sMenuHeight = popup.getHeight();
    }
}
