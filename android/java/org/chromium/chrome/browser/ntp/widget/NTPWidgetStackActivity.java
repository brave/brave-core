/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp.widget;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.cardview.widget.CardView;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.ntp.BraveNewTabPageLayout;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetStackAdapter;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;

import java.util.ArrayList;
import java.util.List;

public class NTPWidgetStackActivity extends AsyncInitializationActivity {
    private NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener;
    private RecyclerView usedWidgetsRecyclerView;
    private RecyclerView availableWidgetsRecyclerView;
    private NTPWidgetStackAdapter usedNTPWidgetStackAdapter;
    private NTPWidgetStackAdapter availableNTPWidgetStackAdapter;
    private LinearLayout availableWidgetLayout;
    private CardView usedWidgetLayout;
    private boolean isFromSettings;
    private boolean mNativeInitialized;
    public static final int USED_WIDGET = 0;
    public static final int AVAILABLE_WIDGET = 1;
    public static final String FROM_SETTINGS = "from_settings";

    public interface NTPWidgetStackUpdateListener {
        void onAddToWidget(String widget);
        void onRemoveFromWidget(String widget);
    }

    public void setNTPWidgetListener(NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener) {
        this.ntpWidgetListener = ntpWidgetListener;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_ntp_widget_stack);

        if (getIntent() != null) {
            isFromSettings = getIntent().getBooleanExtra("from_settings", false);
        }

        List<String> usedWidgetList = NTPWidgetManager.getInstance().getUsedWidgets();
        List<String> availableWidgetList = NTPWidgetManager.getInstance().getAvailableWidgets();

        ImageView backButton = findViewById(R.id.back_button);
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                returnResult();
            }
        });

        usedWidgetLayout = findViewById(R.id.used_widget_layout);

        usedWidgetsRecyclerView = findViewById(R.id.used_widget_list);
        usedWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        usedNTPWidgetStackAdapter = new NTPWidgetStackAdapter();
        SwipeAndDragHelper swipeAndDragHelper = new SwipeAndDragHelper(usedNTPWidgetStackAdapter);
        ItemTouchHelper touchHelper = new ItemTouchHelper(swipeAndDragHelper);
        usedNTPWidgetStackAdapter.setTouchHelper(touchHelper);
        usedNTPWidgetStackAdapter.setNTPWidgetListener(ntpWidgetListener);
        usedNTPWidgetStackAdapter.setNTPWidgetStackUpdateListener(ntpWidgetStackUpdateListener);
        usedNTPWidgetStackAdapter.setNTPWidgetType(USED_WIDGET);
        usedWidgetsRecyclerView.setAdapter(usedNTPWidgetStackAdapter);
        touchHelper.attachToRecyclerView(usedWidgetsRecyclerView);
        usedNTPWidgetStackAdapter.setWidgetList(usedWidgetList);

        availableWidgetLayout = findViewById(R.id.available_widget_layout);

        availableWidgetsRecyclerView = findViewById(R.id.available_widget_list);
        availableWidgetsRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        availableNTPWidgetStackAdapter = new NTPWidgetStackAdapter();
        availableNTPWidgetStackAdapter.setNTPWidgetListener(ntpWidgetListener);
        availableNTPWidgetStackAdapter.setNTPWidgetStackUpdateListener(
                ntpWidgetStackUpdateListener);
        availableNTPWidgetStackAdapter.setNTPWidgetType(AVAILABLE_WIDGET);
        availableWidgetsRecyclerView.setAdapter(availableNTPWidgetStackAdapter);
        availableNTPWidgetStackAdapter.setWidgetList(availableWidgetList);
        updateNTPWidgetStackLayout();

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        mNativeInitialized = true;
    }

    @Override
    public void onBackPressed() {
        returnResult();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    private NTPWidgetStackUpdateListener ntpWidgetStackUpdateListener =
            new NTPWidgetStackUpdateListener() {
                @Override
                public void onAddToWidget(String widget) {
                    usedNTPWidgetStackAdapter.addWidget(widget);
                    updateNTPWidgetStackLayout();
                }

                @Override
                public void onRemoveFromWidget(String widget) {
                    availableNTPWidgetStackAdapter.addWidget(widget);
                    updateNTPWidgetStackLayout();
                }
            };

    private void updateNTPWidgetStackLayout() {
        if (availableNTPWidgetStackAdapter != null && availableWidgetLayout != null) {
            if (availableNTPWidgetStackAdapter.getWidgetList().size() > 0) {
                availableWidgetLayout.setVisibility(View.VISIBLE);
            } else {
                availableWidgetLayout.setVisibility(View.GONE);
            }
        }

        if (usedNTPWidgetStackAdapter != null && usedWidgetLayout != null) {
            if (usedNTPWidgetStackAdapter.getWidgetList().size() > 0) {
                usedWidgetLayout.setVisibility(View.VISIBLE);
            } else {
                usedWidgetLayout.setVisibility(View.GONE);
            }
        }
    }

    private void returnResult() {
        for (int i = 0; i < usedNTPWidgetStackAdapter.getWidgetList().size(); i++) {
            NTPWidgetManager.getInstance().setWidget(
                    usedNTPWidgetStackAdapter.getWidgetList().get(i), i);
        }
        for (int i = 0; i < availableNTPWidgetStackAdapter.getWidgetList().size(); i++) {
            NTPWidgetManager.getInstance().setWidget(
                    availableNTPWidgetStackAdapter.getWidgetList().get(i), -1);
            if (availableNTPWidgetStackAdapter.getWidgetList().get(i).equals(
                        NTPWidgetManager.PREF_BINANCE)
                    && mNativeInitialized) {
                BinanceNativeWorker.getInstance().revokeToken();
                BinanceWidgetManager.getInstance().setBinanceAccountBalance("");
                BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(false);
            }
        }
        if (isFromSettings) {
            if (BraveActivity.getBraveActivity() != null && BraveActivity.getBraveActivity().getActivityTab() != null) {
                BraveActivity.getBraveActivity().getActivityTab().reloadIgnoringCache();
            }
        } else {
            Intent intent = new Intent();
            setResult(BraveNewTabPageLayout.NTP_WIDGET_STACK_CODE, intent);
        }
        finish();
    }
}
