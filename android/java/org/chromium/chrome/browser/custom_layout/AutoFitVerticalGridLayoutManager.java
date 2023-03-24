/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.custom_layout;

import android.content.Context;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.ui.base.ViewUtils;

/**
 * Extension of {@link GridLayoutManager} which adapts span count to recycler view size.
 */
public class AutoFitVerticalGridLayoutManager extends GridLayoutManager {
    private final int minColumns;

    private int columnWidthPx;
    private int lastKnownWidth;

    /**
     * @param minColumns Minimum columns count.
     * @param columnWidthDp Expected width of the single item in grid layout in density indipendent
     *         pixels.
     */
    public AutoFitVerticalGridLayoutManager(
            final Context context, final int minColumns, final float columnWidthDp) {
        super(context, 1);
        this.minColumns = minColumns;
        setcolumnWidthPx(ViewUtils.dpToPx(context, columnWidthDp));
        setOrientation(RecyclerView.VERTICAL);
    }

    private void setcolumnWidthPx(final int newColumnWidthPx) {
        if (newColumnWidthPx > 0 && newColumnWidthPx != columnWidthPx) {
            columnWidthPx = newColumnWidthPx;
            lastKnownWidth = 0;
        }
    }

    @Override
    public void onLayoutChildren(
            final RecyclerView.Recycler recycler, final RecyclerView.State state) {
        final int currentWidth = getWidth() - getPaddingRight() - getPaddingLeft();
        if (columnWidthPx > 0 && lastKnownWidth != currentWidth) {
            final int totalSpace;
            totalSpace = getWidth() - getPaddingRight() - getPaddingLeft();
            final int spanCount = Math.max(minColumns, totalSpace / columnWidthPx);
            setSpanCount(spanCount);
            lastKnownWidth = currentWidth;
        }
        super.onLayoutChildren(recycler, state);
    }
}
