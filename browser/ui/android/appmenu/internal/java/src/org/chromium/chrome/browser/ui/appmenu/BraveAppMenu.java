/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.content.SharedPreferences;
import android.graphics.Rect;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ui.appmenu.internal.R;
import org.chromium.ui.base.DeviceFormFactor;

class BraveAppMenu {
    private static final String BRAVE_BOTTOM_TOOLBAR_CURRENTLY_VISIBLE =
            "brave_bottom_toolbar_currently_visible";

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
        return isMenuFromBottom() ? R.style.OverflowMenuAnimBottom : R.style.OverflowMenuAnim;
    }

    private static boolean isMenuFromBottom() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BRAVE_BOTTOM_TOOLBAR_CURRENTLY_VISIBLE, false);
    }
}
