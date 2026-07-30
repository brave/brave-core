/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;

import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;
import org.chromium.chrome.browser.util.ConfigurationUtils;

import java.util.List;

/**
 * A general purpose fragment representing a list of Items where each item containing a title and
 * sub-title.
 */
@NullMarked
public class TwoLineItemBottomSheetFragment extends WalletBottomSheetDialogFragment {
    @MonotonicNonNull private List<TwoLineItem> mItems;
    @MonotonicNonNull private TwoLineItemRecyclerViewAdapter mAdapter;

    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        // In landscape the sheet would otherwise open at its collapsed peek height, showing
        // only the title. Expand it on show so the details list is visible and scrollable.
        dialog.setOnShowListener(shownDialog -> expandInLandscape((BottomSheetDialog) shownDialog));
        return dialog;
    }

    private void expandInLandscape(BottomSheetDialog bottomSheetDialog) {
        if (!ConfigurationUtils.isLandscape(requireContext())) {
            return;
        }
        final BottomSheetBehavior<FrameLayout> behavior = bottomSheetDialog.getBehavior();
        behavior.setSkipCollapsed(true);
        // Defer expanding by one frame. Expanding synchronously from the show listener can run
        // before the landscape system-bar insets are dispatched, so BottomSheetBehavior lays the
        // sheet out edge-to-edge (insetLeft == 0) and its content is clipped under the navigation
        // bar. By the next frame the insets are applied and the expanded sheet is offset correctly.
        final View content = getView();
        if (content != null) {
            content.post(() -> behavior.setState(BottomSheetBehavior.STATE_EXPANDED));
        } else {
            behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_two_line_item_sheet, container, false);

        // Phones drop the Material width cap (WalletBottomSheetDialogFragment.onStart), so the
        // sheet is full-width and can run under a display cutout; keep its contents clear of it.
        // The cutout's left/right insets are already orientation-aware (zero unless the cutout is
        // on a vertical edge), so no orientation check is needed. Tablets keep the centered,
        // width-limited sheet, which never reaches a screen-edge cutout, so they are excluded.
        // BottomSheetBehavior already applies the system-bar insets, so only the cutout is here.
        final int basePaddingLeft = view.getPaddingLeft();
        final int basePaddingRight = view.getPaddingRight();
        ViewCompat.setOnApplyWindowInsetsListener(
                view,
                (v, insets) -> {
                    int left = basePaddingLeft;
                    int right = basePaddingRight;
                    if (!ConfigurationUtils.isTablet(v.getContext())) {
                        final Insets cutout =
                                insets.getInsets(WindowInsetsCompat.Type.displayCutout());
                        left += cutout.left;
                        right += cutout.right;
                    }
                    v.setPadding(left, v.getPaddingTop(), right, v.getPaddingBottom());
                    return insets;
                });

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
