/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NTPWidgetItem;

import java.util.ArrayList;
import java.util.List;

public class NTPWidgetBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private List<NTPWidgetItem> usersList = new ArrayList<NTPWidgetItem>() {
        {
            add(new NTPWidgetItem("Privacy Stats",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
            add(new NTPWidgetItem("Favorites",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
            add(new NTPWidgetItem("Brave Rewards",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
            add(new NTPWidgetItem("Binance",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
        }
    };

    static NTPWidgetBottomSheetDialogFragment newInstance() {
        return new NTPWidgetBottomSheetDialogFragment();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_ntp_widget_bottom_sheet_dialog, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        RecyclerView userRecyclerView = view.findViewById(R.id.recyclerview_user_list);
        userRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
        NTPWidgetListAdapter adapter = new NTPWidgetListAdapter();
        SwipeAndDragHelper swipeAndDragHelper = new SwipeAndDragHelper(adapter);
        ItemTouchHelper touchHelper = new ItemTouchHelper(swipeAndDragHelper);
        adapter.setTouchHelper(touchHelper);
        userRecyclerView.setAdapter(adapter);
        touchHelper.attachToRecyclerView(userRecyclerView);

        adapter.setUserList(usersList);
    }
}
