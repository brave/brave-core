/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.custom_layout.popup_window_tooltip;

import android.content.Context;
import android.graphics.RectF;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.ColorRes;

public final class PopupWindowTooltipUtils {
    public static RectF calculeRectOnScreen(View view) {
        int[] location = new int[2];
        view.getLocationOnScreen(location);
        return new RectF(location[0], location[1], location[0] + view.getMeasuredWidth(),
                location[1] + view.getMeasuredHeight());
    }

    public static RectF calculeRectInWindow(View view) {
        int[] location = new int[2];
        view.getLocationInWindow(location);
        return new RectF(location[0], location[1], location[0] + view.getMeasuredWidth(),
                location[1] + view.getMeasuredHeight());
    }

    public static void setWidth(View view, float width) {
        ViewGroup.LayoutParams params = view.getLayoutParams();
        if (params == null) {
            params = new ViewGroup.LayoutParams((int) width, view.getHeight());
        } else {
            params.width = (int) width;
        }
        view.setLayoutParams(params);
    }

    public static int tooltipGravityToArrowDirection(int tooltipGravity) {
        switch (tooltipGravity) {
            case Gravity.START:
                return ArrowColorDrawable.RIGHT;
            case Gravity.END:
                return ArrowColorDrawable.LEFT;
            case Gravity.TOP:
                return ArrowColorDrawable.BOTTOM;
            case Gravity.BOTTOM:
                return ArrowColorDrawable.TOP;
            case Gravity.CENTER:
                return ArrowColorDrawable.TOP;
            default:
                throw new IllegalArgumentException(
                        "Gravity must have be CENTER, START, END, TOP or BOTTOM.");
        }
    }

    private static ViewGroup.MarginLayoutParams getOrCreateMarginLayoutParams(View view) {
        ViewGroup.LayoutParams lp = view.getLayoutParams();
        if (lp != null) {
            if (lp instanceof ViewGroup.MarginLayoutParams) {
                return (ViewGroup.MarginLayoutParams) lp;
            } else {
                return new ViewGroup.MarginLayoutParams(lp);
            }
        } else {
            return new ViewGroup.MarginLayoutParams(view.getWidth(), view.getHeight());
        }
    }

    public static int getColor(Context context, @ColorRes int colorRes) {
        return context.getColor(colorRes);
    }

    public static ViewGroup findFrameLayout(View anchorView) {
        ViewGroup rootView = (ViewGroup) anchorView.getRootView();
        if (rootView.getChildCount() == 1 && rootView.getChildAt(0) instanceof FrameLayout) {
            rootView = (ViewGroup) rootView.getChildAt(0);
        }
        return rootView;
    }
}
