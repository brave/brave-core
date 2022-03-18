/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts.components;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.RectF;

import org.chromium.base.MathUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;

public class StackLayoutTab extends LayoutTab {
    public static final WritableFloatPropertyKey TILT_X_IN_DEGREES = new WritableFloatPropertyKey();
    public static final WritableFloatPropertyKey TILT_Y_IN_DEGREES = new WritableFloatPropertyKey();
    public static final WritableFloatPropertyKey TILT_X_PIVOT_OFFSET =
            new WritableFloatPropertyKey();
    public static final WritableFloatPropertyKey TILT_Y_PIVOT_OFFSET =
            new WritableFloatPropertyKey();
    public static final WritableFloatPropertyKey BORDER_CLOSE_BUTTON_ALPHA =
            new WritableFloatPropertyKey();

    public StackLayoutTab(int tabId, boolean isIncognito, int maxContentTextureWidth,
            int maxContentTextureHeight, boolean showCloseButton, boolean isTitleNeeded) {
        super(tabId, isIncognito, maxContentTextureWidth, maxContentTextureHeight);

        initStack(maxContentTextureWidth, maxContentTextureHeight, showCloseButton, isTitleNeeded);
    }

    public void initStack(int maxContentTextureWidth, int maxContentTextureHeight,
            boolean showCloseButton, boolean isTitleNeeded) {
        init(maxContentTextureWidth, maxContentTextureHeight);

        set(BORDER_CLOSE_BUTTON_ALPHA, showCloseButton ? 1.f : 0.f);
        set(TILT_X_IN_DEGREES, 0.0f);
        set(TILT_Y_IN_DEGREES, 0.0f);
        set(IS_TITLE_NEEDED, isTitleNeeded);
    }

    /**
     * @param tilt        The tilt angle around the X axis of the tab in degree.
     * @param pivotOffset The offset of the X axis of the tilt pivot.
     */
    public void setTiltX(float tilt, float pivotOffset) {
        set(TILT_X_IN_DEGREES, tilt);
        set(TILT_X_PIVOT_OFFSET, pivotOffset);
    }

    /**
     * @return The tilt angle around the X axis of the tab in degree.
     */
    public float getTiltX() {
        return get(TILT_X_IN_DEGREES);
    }

    /**
     * @return The offset of the X axis of the tilt pivot.
     */
    public float getTiltXPivotOffset() {
        return get(TILT_X_PIVOT_OFFSET);
    }

    /**
     * @param tilt        The tilt angle around the Y axis of the tab in degree.
     * @param pivotOffset The offset of the Y axis of the tilt pivot.
     */
    public void setTiltY(float tilt, float pivotOffset) {
        set(TILT_Y_IN_DEGREES, tilt);
        set(TILT_Y_PIVOT_OFFSET, pivotOffset);
    }

    /**
     * @return The tilt angle around the Y axis of the tab in degree.
     */
    public float getTiltY() {
        return get(TILT_Y_IN_DEGREES);
    }

    /**
     * @return The offset of the Y axis of the tilt pivot.
     */
    public float getTiltYPivotOffset() {
        return get(TILT_Y_IN_DEGREES);
    }

    /**
     * @param alpha The maximum alpha value of the close button on the border.
     */
    public void setBorderCloseButtonAlpha(float alpha) {
        set(BORDER_CLOSE_BUTTON_ALPHA, alpha);
    }

    /**
     * @return The current alpha value at which the close button on the border is drawn.
     */
    public float getBorderCloseButtonAlpha() {
        return get(BORDER_CLOSE_BUTTON_ALPHA);
    }

    /**
     * @return Whether the tab title should be displayed.
     */
    public boolean isTitleNeeded() {
        return get(IS_TITLE_NEEDED);
    }
}
