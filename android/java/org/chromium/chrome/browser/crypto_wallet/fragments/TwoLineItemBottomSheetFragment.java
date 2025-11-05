/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import androidx.recyclerview.widget.RecyclerView;

import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;

import java.util.List;

/**
 * A general purpose fragment representing a list of Items where each item containing a title and
 * sub-title.
 */
@NullMarked
public class TwoLineItemBottomSheetFragment extends WalletBottomSheetDialogFragment {
    @MonotonicNonNull private List<TwoLineItem> mItems;
    @MonotonicNonNull private TwoLineItemRecyclerViewAdapter mAdapter;

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_two_line_item_sheet, container, false);
        RecyclerView recyclerView = view.findViewById(R.id.frag_two_line_sheet_list);
        if (mAdapter != null) {
            recyclerView.setAdapter(mAdapter);
        }
        final ImageButton closeButton = view.findViewById(R.id.frag_two_line_sheet_ib_close);
        closeButton.setOnClickListener(v -> dismiss());
        recyclerView.setOnTouchListener(
                (v, event) -> {
                    int action = event.getAction();
                    switch (action) {
                        case MotionEvent.ACTION_DOWN:
                            // Disallow NestedScrollView to intercept touch events.
                            v.getParent().requestDisallowInterceptTouchEvent(true);
                            break;

                        case MotionEvent.ACTION_UP:
                            // Allow NestedScrollView to intercept touch events.
                            v.getParent().requestDisallowInterceptTouchEvent(false);
                            break;
                    }

                    // Handle RecyclerView touch events.
                    v.onTouchEvent(event);
                    return true;
                });
        return view;
    }

    public void setItems(List<TwoLineItem> items) {
        mItems = items;
        mAdapter =
                new TwoLineItemRecyclerViewAdapter(
                        mItems, TwoLineItemRecyclerViewAdapter.AdapterViewOrientation.HORIZONTAL);
        mAdapter.mSubTextAlignment = View.TEXT_ALIGNMENT_TEXT_START;
    }
}
