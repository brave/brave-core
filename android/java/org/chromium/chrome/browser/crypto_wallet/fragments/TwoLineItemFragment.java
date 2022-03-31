/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemDataSource;

import java.util.List;

/**
 * A general purpose fragment representing a list of Items where each item containing a title and
 * sub-title.
 */
public class TwoLineItemFragment extends Fragment {
    private List<TwoLineItemDataSource> items;

    public TwoLineItemFragment(List<TwoLineItemDataSource> items) {
        this.items = items;
    }

    public static TwoLineItemFragment newInstance(List<TwoLineItemDataSource> items) {
        TwoLineItemFragment fragment = new TwoLineItemFragment(items);
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_two_line_item_list, container, false);

        RecyclerView recyclerView = (RecyclerView) view;
        recyclerView.setAdapter(new TwoLineItemRecyclerViewAdapter(items));
        return view;
    }
}
