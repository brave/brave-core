/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.widget;

import android.annotation.SuppressLint;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetStackActivity;
import org.chromium.chrome.browser.ntp.widget.SwipeAndDragHelper;

import java.util.List;

public class NTPWidgetStackAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
        implements SwipeAndDragHelper.ActionCompletionContract {
    private List<String> widgetList;
    private ItemTouchHelper touchHelper;
    private NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener;
    private NTPWidgetStackActivity.NTPWidgetStackUpdateListener ntpWidgetStackUpdateListener;
    private int ntpWidgetType;

    public void setNTPWidgetListener(NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener) {
        this.ntpWidgetListener = ntpWidgetListener;
    }

    public void setNTPWidgetStackUpdateListener(
            NTPWidgetStackActivity.NTPWidgetStackUpdateListener ntpWidgetStackUpdateListener) {
        this.ntpWidgetStackUpdateListener = ntpWidgetStackUpdateListener;
    }

    public void setNTPWidgetType(int ntpWidgetType) {
        this.ntpWidgetType = ntpWidgetType;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.ntp_widget_list_item_layout, parent, false);
        return new NTPWidgetViewHolder(view);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onBindViewHolder(@NonNull final RecyclerView.ViewHolder holder, int position) {
        NTPWidgetViewHolder ntpWidgetViewHolder = (NTPWidgetViewHolder) holder;
        NTPWidgetItem ntpWidgetItem = NTPWidgetManager.mWidgetsMap.get(widgetList.get(position));
        ntpWidgetViewHolder.widgetTitle.setText(ntpWidgetItem.getWidgetTitle());
        ntpWidgetViewHolder.widgetText.setText(ntpWidgetItem.getWidgetText());
        if (ntpWidgetType == NTPWidgetStackActivity.USED_WIDGET) {
            ntpWidgetViewHolder.widgetReorderView.setImageResource(R.drawable.ic_sort);
            ntpWidgetViewHolder.widgetReorderView.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                        touchHelper.startDrag(holder);
                    }
                    return false;
                }
            });
        } else if (ntpWidgetType == NTPWidgetStackActivity.AVAILABLE_WIDGET) {
            ntpWidgetViewHolder.widgetReorderView.setImageResource(R.drawable.ic_add);
            ntpWidgetViewHolder.widgetReorderView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    int currentPosition = ntpWidgetViewHolder.getAdapterPosition();
                    String widget = widgetList.get(currentPosition);
                    removeWidgetFromList(currentPosition);
                    ntpWidgetStackUpdateListener.onAddToWidget(widget);
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return widgetList == null ? 0 : widgetList.size();
    }

    public void setWidgetList(List<String> widgetList) {
        this.widgetList = widgetList;
    }

    public List<String> getWidgetList() {
        return this.widgetList;
    }

    public void addWidget(String widget) {
        this.widgetList.add(widget);
        notifyItemInserted(this.widgetList.indexOf(widget));
    }

    public void addWidgetToPosition(int position, String widget) {
        this.widgetList.add(position, widget);
        notifyItemInserted(position);
    }

    @Override
    public void onViewMoved(int oldPosition, int newPosition) {
        String targetWidget = widgetList.get(oldPosition);
        String newWidget = targetWidget;
        widgetList.remove(oldPosition);
        widgetList.add(newPosition, newWidget);
        notifyItemMoved(oldPosition, newPosition);
    }

    @Override
    public void onViewSwiped(int position) {
        String widget = widgetList.get(position);
        removeWidgetFromList(position);
        ntpWidgetStackUpdateListener.onRemoveFromWidget(widget);
    }

    private void removeWidgetFromList(int position) {
        widgetList.remove(position);
        notifyItemRemoved(position);
    }

    void setTouchHelper(ItemTouchHelper touchHelper) {
        this.touchHelper = touchHelper;
    }

    private class NTPWidgetViewHolder extends RecyclerView.ViewHolder {
        TextView widgetTitle;
        TextView widgetText;
        ImageView widgetReorderView;

        NTPWidgetViewHolder(View itemView) {
            super(itemView);
            widgetTitle = itemView.findViewById(R.id.widget_title);
            widgetText = itemView.findViewById(R.id.widget_text);
            widgetReorderView = itemView.findViewById(R.id.widget_reorder);
        }
    }
}
