/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;

import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

/** Extension to {@link GridLayoutManager} which adapts its columns to recycler view size. */
public class AutoGridLayoutManager extends GridLayoutManager {

    private final int mMinColumns;

    private int mColumnWidth;
    private int mLastKnownWidth;

    /**
     * @param minColumns Minimum columns count.
     * @param columnWidth Expected width in pixels of the single item in grid layout.
     */
    public AutoGridLayoutManager(
            final Context context, final int minColumns, final int columnWidth) {
        super(context, 1);
        mLastKnownWidth = 0;
        mMinColumns = minColumns;
        setColumnWidth(columnWidth);
        setOrientation(RecyclerView.VERTICAL);
    }

    private void setColumnWidth(final int newColumnWidth) {
        if (newColumnWidth > 0 && newColumnWidth != mColumnWidth) {
            mColumnWidth = newColumnWidth;
            mLastKnownWidth = 0;
        }
    }

    public int calculateSpanCount() {
        final int totalSpace;
        totalSpace = getWidth() - getPaddingRight() - getPaddingLeft();
        return Math.max(mMinColumns, totalSpace / mColumnWidth);
    }

    @Override
    public void onLayoutChildren(
            final RecyclerView.Recycler recycler, final RecyclerView.State state) {
        final int currentWidth = getWidth() - getPaddingRight() - getPaddingLeft();
        if (mColumnWidth > 0 && mLastKnownWidth != currentWidth) {
            final int spanCount = calculateSpanCount();
            setSpanCount(spanCount);
            mLastKnownWidth = currentWidth;
        }
        super.onLayoutChildren(recycler, state);
    }
}
