/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.timer;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.List;

public class TimerItemAdapter extends RecyclerView.Adapter<TimerItemViewHolder> {
    public interface TimerItemClickListener {
        void onTimerItemClick(TimerItemModel timerItemModel);
    }

    private final List<TimerItemModel> mTimerItemModels;
    private final TimerItemClickListener mTimerItemClickListener;

    TimerItemAdapter(
            List<TimerItemModel> timerItemModels, TimerItemClickListener timerItemClickListener) {
        mTimerItemModels = timerItemModels;
        mTimerItemClickListener = timerItemClickListener;
    }

    @NonNull
    @Override
    public TimerItemViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View timerItem =
                layoutInflater.inflate(
                        R.layout.fragment_timer_dialog_list_dialog_item, parent, false);
        return new TimerItemViewHolder(timerItem);
    }

    @Override
    public void onBindViewHolder(TimerItemViewHolder holder, int position) {
        TimerItemModel timerItemModel = mTimerItemModels.get(position);
        holder.mTimerActionText.setText(timerItemModel.getActionText());
        holder.mTimerActionImage.setImageResource(timerItemModel.getActionImage());
        holder.itemView.setOnClickListener(
                view -> mTimerItemClickListener.onTimerItemClick(timerItemModel));
    }

    @Override
    public int getItemCount() {
        return mTimerItemModels.size();
    }
}
