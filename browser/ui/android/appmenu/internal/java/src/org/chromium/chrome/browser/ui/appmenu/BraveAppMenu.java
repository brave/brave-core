/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Rect;
import android.view.View;
import android.widget.PopupWindow;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.SysUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ui.appmenu.internal.R;
import org.chromium.ui.hierarchicalmenu.HierarchicalMenuController;

@NullMarked
class BraveAppMenu extends BraveAppMenuDummySuper {
    private static final int BOTTOM_MENU_VERTICAL_OFFSET_DP = 44;
    private static int sMenuHeight;
    private static int sNegativeVerticalOffsetNotTopAnchored;

    BraveAppMenu(
            AppMenuHandlerImpl handler,
            Resources res,
            HierarchicalMenuController hierarchicalMenuController) {
        super(handler, res, hierarchicalMenuController);

        final float scale = res.getDisplayMetrics().density;
        sNegativeVerticalOffsetNotTopAnchored =
                (int) (BOTTOM_MENU_VERTICAL_OFFSET_DP * scale + 0.5f);
    }

    @Nullable
    @Override
    public View createAppMenuContentView(Context context, boolean addTopPaddingBeforeFirstRow) {
        return maybeRemovePaddingFromBottom(
                super.createAppMenuContentView(context, addTopPaddingBeforeFirstRow));
    }

    @Nullable
    private View maybeRemovePaddingFromBottom(@Nullable View contentView) {
        if (!isMenuFromBottom() || contentView == null) return contentView;

        // Remove the padding from the bottom of the content view
        contentView.setPadding(
                contentView.getPaddingLeft(),
                contentView.getPaddingTop(),
                contentView.getPaddingRight(),
                0);

        return contentView;
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
        if (!SysUtils.isLowEndDevice() && !isByPermanentButton) {
            popup.setAnimationStyle(BraveAppMenu.getAnimationStyle());
        }
        sMenuHeight = popup.getHeight();
    }
}
