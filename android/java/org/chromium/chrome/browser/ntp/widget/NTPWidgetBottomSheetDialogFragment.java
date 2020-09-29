/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.widget;

import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetListAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;

import java.util.ArrayList;
import java.util.List;

public class NTPWidgetBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener;
    private RecyclerView usedWidgetsRecyclerView;
    private RecyclerView availableWidgetsRecyclerView;
    private NTPWidgetListAdapter usedNtpWidgetListAdapter;
    private NTPWidgetListAdapter availableNtpWidgetListAdapter;
    private LinearLayout availableWidgetLayout;
    public static final int USED_WIDGET = 0;
    public static final int AVAILABLE_WIDGET = 1;

    public interface NTPWidgetStackUpdateListener {
        void onWidgetStackUpdate();
    }

    public void setNTPWidgetListener(NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener) {
        this.ntpWidgetListener = ntpWidgetListener;
    }

    public static NTPWidgetBottomSheetDialogFragment newInstance() {
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

        List<String> usedWidgetList = NTPWidgetManager.getInstance().getUsedWidgets();
        List<String> availableWidgetList = NTPWidgetManager.getInstance().getAvailableWidgets();

        usedWidgetsRecyclerView = view.findViewById(R.id.used_widget_list);
        usedWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
        usedNtpWidgetListAdapter = new NTPWidgetListAdapter();
        SwipeAndDragHelper swipeAndDragHelper = new SwipeAndDragHelper(usedNtpWidgetListAdapter);
        ItemTouchHelper touchHelper = new ItemTouchHelper(swipeAndDragHelper);
        usedNtpWidgetListAdapter.setTouchHelper(touchHelper);
        usedNtpWidgetListAdapter.setNTPWidgetListener(ntpWidgetListener);
        usedNtpWidgetListAdapter.setNTPWidgetStackUpdateListener(ntpWidgetStackUpdateListener);
        usedNtpWidgetListAdapter.setNTPWidgetType(USED_WIDGET);
        usedWidgetsRecyclerView.setAdapter(usedNtpWidgetListAdapter);
        touchHelper.attachToRecyclerView(usedWidgetsRecyclerView);
        usedNtpWidgetListAdapter.setWidgetList(usedWidgetList);

        availableWidgetLayout = view.findViewById(R.id.available_widget_layout);

        availableWidgetsRecyclerView = view.findViewById(R.id.available_widget_list);
        availableWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
        availableNtpWidgetListAdapter = new NTPWidgetListAdapter();
        availableNtpWidgetListAdapter.setNTPWidgetListener(ntpWidgetListener);
        availableNtpWidgetListAdapter.setNTPWidgetStackUpdateListener(ntpWidgetStackUpdateListener);
        availableNtpWidgetListAdapter.setNTPWidgetType(AVAILABLE_WIDGET);
        availableWidgetsRecyclerView.setAdapter(availableNtpWidgetListAdapter);
        availableNtpWidgetListAdapter.setWidgetList(availableWidgetList);
        if (availableWidgetList.size() > 0) {
            availableWidgetLayout.setVisibility(View.VISIBLE);
        } else {
            availableWidgetLayout.setVisibility(View.GONE);
        }
    }

    @Override
    public void onDismiss(@NonNull DialogInterface dialog) {
        super.onDismiss(dialog);
        for (int i = 0; i < usedNtpWidgetListAdapter.getWidgetList().size(); i++) {
            NTPWidgetManager.getInstance().setWidget(
                    usedNtpWidgetListAdapter.getWidgetList().get(i), i);
        }
        ntpWidgetListener.onBottomSheetDismiss();
    }

    private NTPWidgetStackUpdateListener ntpWidgetStackUpdateListener =
            new NTPWidgetStackUpdateListener() {
                @Override
                public void onWidgetStackUpdate() {
                    List<String> usedWidgetList = NTPWidgetManager.getInstance().getUsedWidgets();
                    List<String> availableWidgetList =
                            NTPWidgetManager.getInstance().getAvailableWidgets();

                    usedNtpWidgetListAdapter.setWidgetList(usedWidgetList);
                    usedNtpWidgetListAdapter.notifyDataSetChanged();

                    if (availableWidgetList.size() > 0) {
                        availableWidgetLayout.setVisibility(View.VISIBLE);
                        availableNtpWidgetListAdapter.setWidgetList(availableWidgetList);
                        availableNtpWidgetListAdapter.notifyDataSetChanged();
                    } else {
                        availableWidgetLayout.setVisibility(View.GONE);
                    }
                }
            };
}
