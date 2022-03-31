/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Rect;
import android.view.Menu;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ui.appmenu.internal.R;
import org.chromium.ui.base.DeviceFormFactor;

class BraveAppMenu extends AppMenu {
    private static final String BRAVE_IS_MENU_FROM_BOTTOM = "brave_is_menu_from_bottom";

    BraveAppMenu(int itemRowHeight, AppMenuHandlerImpl handler, Resources res) {
        super(itemRowHeight, handler, res);
    }

    @SuppressLint("VisibleForTests")
    public static int[] getPopupPosition(int[] tempLocation, boolean isByPermanentButton,
            int negativeSoftwareVerticalOffset, int negativeVerticalOffsetNotTopAnchored,
            int screenRotation, Rect appRect, Rect padding, View anchorView, int popupWidth,
            int popupHeight, int viewLayoutDirection) {
        int[] position = AppMenu.getPopupPosition(tempLocation, isByPermanentButton,
                negativeSoftwareVerticalOffset, negativeVerticalOffsetNotTopAnchored,
                screenRotation, appRect, padding, anchorView, popupWidth, popupHeight,
                viewLayoutDirection);
        if (isMenuFromBottom()) {
            anchorView.getLocationOnScreen(tempLocation);
            int anchorViewLocationOnScreenY = tempLocation[1];
            position[1] += appRect.bottom - anchorViewLocationOnScreenY - popupHeight;
            position[1] -= negativeVerticalOffsetNotTopAnchored;
            position[1] += padding.bottom;
        }
        return position;
    }

    public static int getAnimationStyle() {
        return isMenuFromBottom() ? R.style.EndIconMenuAnimBottom : R.style.EndIconMenuAnim;
    }

    private static boolean isMenuFromBottom() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BRAVE_IS_MENU_FROM_BOTTOM, false);
    }

    public void runMenuItemEnterAnimations() {
        // We do nothing here as we don't want any fancy animation for the menu.
    }
}
