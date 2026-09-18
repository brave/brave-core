/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesAdapter;

public class ItemTouchHelperCallback extends ItemTouchHelper.Callback {
    private static final int NO_POSITION = RecyclerView.NO_POSITION;

    private final QuickSearchEnginesAdapter mQuickSearchAdapter;

    private int mStartPosition = NO_POSITION;

    public ItemTouchHelperCallback(QuickSearchEnginesAdapter quickSearchAdapter) {
        this.mQuickSearchAdapter = quickSearchAdapter;
    }

    @Override
    public int getMovementFlags(
            @NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder) {
        int dragFlags = ItemTouchHelper.UP | ItemTouchHelper.DOWN;
        int swipeFlags = 0;
        return makeMovementFlags(dragFlags, swipeFlags);
    }

    @Override
    public boolean onMove(
            @NonNull RecyclerView recyclerView,
            @NonNull RecyclerView.ViewHolder viewHolder,
            @NonNull RecyclerView.ViewHolder target) {
        int fromPosition = viewHolder.getBindingAdapterPosition();
        int toPosition = target.getBindingAdapterPosition();
        if (fromPosition == NO_POSITION || toPosition == NO_POSITION) {
            return false;
        }
        mQuickSearchAdapter.moveItem(fromPosition, toPosition);
        return true;
    }

    @Override
    public void onSwiped(@NonNull RecyclerView.ViewHolder viewHolder, int direction) {
        // No swiping action
    }

    @Override
    public void onSelectedChanged(@Nullable RecyclerView.ViewHolder viewHolder, int actionState) {
        super.onSelectedChanged(viewHolder, actionState);
        if (actionState == ItemTouchHelper.ACTION_STATE_DRAG
                && viewHolder != null
                && mStartPosition == NO_POSITION) {
            mStartPosition = viewHolder.getBindingAdapterPosition();
        }
    }

    @Override
    public void clearView(
            @NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder) {
        super.clearView(recyclerView, viewHolder);
        int currentPosition = viewHolder.getBindingAdapterPosition();
        boolean hasMoved =
                mStartPosition != NO_POSITION
                        && currentPosition != NO_POSITION
                        && currentPosition != mStartPosition;
        mStartPosition = NO_POSITION;
        if (!hasMoved || !recyclerView.isAttachedToWindow()) {
            return;
        }
        // Save the new order once the drop animation is done and RecyclerView has finished
        // laying out, the same way DragTouchHandler commits its swaps.
        recyclerView.post(mQuickSearchAdapter::onOrderChanged);
    }
}
