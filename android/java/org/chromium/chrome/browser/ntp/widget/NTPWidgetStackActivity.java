/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp.widget;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.LinearLayout;
import android.widget.ImageView;

import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetListAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;

import java.util.ArrayList;
import java.util.List;

public class NTPWidgetStackActivity extends AppCompatActivity {
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ntp_widget_stack);

        List<String> usedWidgetList = NTPWidgetManager.getInstance().getUsedWidgets();
        List<String> availableWidgetList = NTPWidgetManager.getInstance().getAvailableWidgets();

        ImageView backButton = findViewById(R.id.back_button);
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                finish();
            }
        });

        usedWidgetsRecyclerView = findViewById(R.id.used_widget_list);
        usedWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(this));
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

        availableWidgetLayout = findViewById(R.id.available_widget_layout);

        availableWidgetsRecyclerView = findViewById(R.id.available_widget_list);
        availableWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(this));
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
    protected void onStop() {
        for (int i = 0; i < usedNtpWidgetListAdapter.getWidgetList().size(); i++) {
            NTPWidgetManager.getInstance().setWidget(
                usedNtpWidgetListAdapter.getWidgetList().get(i), i);
        }
        super.onStop();
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
