/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import android.content.Context;
import android.util.AttributeSet;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;

/*
 * Provides a custom LayoutManager to use on recyclerView to prevent an Android bug from crashing.
 * Check that issue and related PR for more information
 * https://github.com/brave/brave-browser/issues/29634
 */
public class LinearLayoutManagerWrapper extends LinearLayoutManager {
    private static final String TAG = "LLManagerWrapper";

    public LinearLayoutManagerWrapper(Context context) {
        super(context);
    }

    public LinearLayoutManagerWrapper(Context context, int orientation, boolean reverseLayout) {
        super(context, orientation, reverseLayout);
    }

    public LinearLayoutManagerWrapper(
            Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    public void onLayoutChildren(RecyclerView.Recycler recycler, RecyclerView.State state) {
        try {
            super.onLayoutChildren(recycler, state);
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "IndexOutOfBoundsException in RecyclerView: ", e);
            assert false;
        } catch (ClassCastException e) {
            Log.e(TAG, "ClassCastException in RecyclerView: ", e);
            assert false;
        }
    }
}
