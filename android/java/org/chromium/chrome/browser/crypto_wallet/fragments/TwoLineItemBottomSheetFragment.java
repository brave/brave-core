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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;

import java.util.List;

/**
 * A general purpose fragment representing a list of Items where each item containing a title and
 * sub-title.
 */
public class TwoLineItemBottomSheetFragment extends WalletBottomSheetDialogFragment {
    private static final String TAG = "TwoLineItemSheetFrag";
    private List<TwoLineItem> items;
    private TwoLineItemRecyclerViewAdapter mAdapter;
    public String mTitle;
    private TextView mTvTitle;
    private ImageButton mIbClose;

    private TwoLineItemBottomSheetFragment() {}

    public TwoLineItemBottomSheetFragment(List<TwoLineItem> items) {
        this.items = items;
    }

    public static TwoLineItemBottomSheetFragment newInstance(List<TwoLineItem> items) {
        TwoLineItemBottomSheetFragment fragment = new TwoLineItemBottomSheetFragment(items);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            registerKeyringObserver(activity.getWalletModel().getKeyringModel());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_two_line_item_sheet, container, false);
        RecyclerView recyclerView = view.findViewById(R.id.frag_two_line_sheet_list);
        mAdapter =
                new TwoLineItemRecyclerViewAdapter(
                        items, TwoLineItemRecyclerViewAdapter.ADAPTER_VIEW_ORIENTATION.HORIZONTAL);
        mAdapter.mSubTextAlignment = View.TEXT_ALIGNMENT_TEXT_START;
        recyclerView.setAdapter(mAdapter);
        mIbClose = view.findViewById(R.id.frag_two_line_sheet_ib_close);
        mIbClose.setOnClickListener(
                v -> {
                    dismiss();
                });
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

    @SuppressLint("NotifyDataSetChanged")
    public void invalidateData() {
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }
}
