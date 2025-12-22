/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.GridLayout;

import java.util.ArrayList;
import java.util.List;

/** The most visited tiles layout. */
public class BraveMostVisitedTilesLayoutBase extends TilesLinearLayout {
    private static final int FIXED_COLUMNS_COUNT = 4;

    private boolean mUseFixedLayout;

    /** Constructor for inflating from XML. */
    public BraveMostVisitedTilesLayoutBase(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setUseFixedLayout(boolean useFixedLayout) {
        mUseFixedLayout = useFixedLayout;
    }

    @SuppressLint("DrawAllocation")
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int childCount = getChildCount();
        if (mUseFixedLayout && widthMeasureSpec > 0 && childCount > FIXED_COLUMNS_COUNT) {
            int numColumns = FIXED_COLUMNS_COUNT;
            if (getColumnCount() < numColumns) {
                setColumnCount(numColumns);
            }
            int numRows = (childCount / numColumns) + 1;
            if (getRowCount() < numRows) {
                setRowCount(numRows);
            }

            List<View> dividerList = new ArrayList<>();

            int tileViewWidth = widthMeasureSpec / numColumns;
            for (int i = 0; i < childCount; i++) {
                View tileView = getChildAt(i);

                if (tileView instanceof SuggestionsTileVerticalDivider) {
                    dividerList.add(tileView);
                    continue;
                }

                int j = i - dividerList.size();
                int row = j / numColumns;
                int column = j % numColumns;
                GridLayout.LayoutParams params =
                        new GridLayout.LayoutParams(
                                GridLayout.spec(row, GridLayout.CENTER, 1),
                                GridLayout.spec(column, GridLayout.CENTER, 1));
                params.width = tileViewWidth;
                updateViewLayout(tileView, params);
            }

            // Add dividers after normal tiles, to make it look better
            for (int i = 0; i < dividerList.size(); i++) {
                int j = childCount - dividerList.size() + i;
                int row = j / numColumns;
                int column = j % numColumns;
                GridLayout.LayoutParams params =
                        new GridLayout.LayoutParams(
                                GridLayout.spec(row, GridLayout.CENTER, 1),
                                GridLayout.spec(column, GridLayout.CENTER, 1));
                params.width = 0;
                updateViewLayout(dividerList.get(i), params);
            }
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    // The methods to be removed with bytecode
    int getColumnCount() {
        assert false;
        return 0;
    }

    void setColumnCount(int columnCount) {
        assert false;
    }

    int getRowCount() {
        assert false;
        return 0;
    }

    void setRowCount(int rowCount) {
        assert false;
    }
}
