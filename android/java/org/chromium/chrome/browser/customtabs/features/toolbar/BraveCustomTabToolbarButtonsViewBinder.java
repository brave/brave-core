/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs.features.toolbar;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;

@NullMarked
public class BraveCustomTabToolbarButtonsViewBinder extends CustomTabToolbarButtonsViewBinder {
    @Override
    public void bind(PropertyModel model, CustomTabToolbar view, PropertyKey propertyKey) {
        super.bind(model, view, propertyKey);

        positionBraveShieldsButton(view);
    }

    private static void positionBraveShieldsButton(CustomTabToolbar view) {
        View shieldsButton = view.findViewById(R.id.brave_shields_button);
        if (shieldsButton == null) return;

        int defaultButtonWidth =
                view.getResources().getDimensionPixelSize(R.dimen.toolbar_button_width);

        // Shields should sit immediately to the left of the menu button.
        // The menu button's end margin is set by upstream and never modified by Brave,
        // so it is a stable anchor for computing all margins absolutely.
        int shieldsEndMargin;
        MenuButton menuButton = view.getMenuButton();
        if (menuButton != null && menuButton.getVisibility() == View.VISIBLE) {
            FrameLayout.LayoutParams menuLp =
                    assumeNonNull((FrameLayout.LayoutParams) menuButton.getLayoutParams());
            shieldsEndMargin = menuLp.getMarginEnd() + defaultButtonWidth;
        } else {
            shieldsEndMargin =
                    view.getResources()
                            .getDimensionPixelSize(R.dimen.custom_tabs_toolbar_horizontal_padding);
        }

        // Set each action button's end margin absolutely from the shields anchor.
        // Upstream adds buttons via addView(btn, 0) in priority order, so index 0
        // is leftmost and index n-1 is rightmost (closest to menu/shields).
        FrameLayout customActionButtons = view.getCustomActionButtonsParent();
        int n = customActionButtons != null ? customActionButtons.getChildCount() : 0;
        if (customActionButtons != null) {
            for (int i = 0; i < n; i++) {
                View child = customActionButtons.getChildAt(i);
                ViewGroup.MarginLayoutParams childLp =
                        assumeNonNull((ViewGroup.MarginLayoutParams) child.getLayoutParams());
                childLp.setMarginEnd(shieldsEndMargin + (n - i) * defaultButtonWidth);
                child.setLayoutParams(childLp);
            }
        }

        // Push the location bar left to make room for the shields button and all action buttons.
        View locationBar = assumeNonNull(view.findViewById(R.id.location_bar_frame_layout));
        ViewGroup.MarginLayoutParams locationBarLp =
                assumeNonNull((ViewGroup.MarginLayoutParams) locationBar.getLayoutParams());
        locationBarLp.setMarginEnd(shieldsEndMargin + (n + 1) * defaultButtonWidth);
        locationBar.setLayoutParams(locationBarLp);

        FrameLayout.LayoutParams lp =
                assumeNonNull((FrameLayout.LayoutParams) shieldsButton.getLayoutParams());
        lp.gravity = Gravity.CENTER_VERTICAL | Gravity.END;
        lp.setMarginEnd(shieldsEndMargin);
        shieldsButton.setLayoutParams(lp);
    }
}
