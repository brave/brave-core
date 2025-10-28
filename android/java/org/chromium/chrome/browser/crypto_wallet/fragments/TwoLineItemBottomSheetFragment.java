/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;

import java.util.List;

/**
 * A general purpose fragment representing a list of Items where each item containing a title and
 * sub-title.
 */
@NullMarked
public class TwoLineItemBottomSheetFragment extends WalletBottomSheetDialogFragment {
    @Nullable
    private List<TwoLineItem> mItems;
    private TwoLineItemRecyclerViewAdapter mAdapter;
    public String mTitle;
    private TextView mTvTitle;
    private ImageButton mIbClose;

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (mItems == null) {
            dismissNow();
        }
        View view = inflater.inflate(R.layout.fragment_two_line_item_sheet, container, false);
        RecyclerView recyclerView = view.findViewById(R.id.frag_two_line_sheet_list);
        mAdapter =
                new TwoLineItemRecyclerViewAdapter(
                        mItems, TwoLineItemRecyclerViewAdapter.AdapterViewOrientation.HORIZONTAL);
        mAdapter.mSubTextAlignment = View.TEXT_ALIGNMENT_TEXT_START;
        recyclerView.setAdapter(mAdapter);
        mIbClose = view.findViewById(R.id.frag_two_line_sheet_ib_close);
        mIbClose.setOnClickListener(
                v -> dismiss());
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
        if (!TextUtils.isEmpty(mTitle)) {
            mTvTitle = view.findViewById(R.id.frag_two_line_sheet_title);
            mTvTitle.setText(mTitle);
        }
        return view;
    }

    public void setItems(List<TwoLineItem> items) {
        mItems = items;
    }
}
