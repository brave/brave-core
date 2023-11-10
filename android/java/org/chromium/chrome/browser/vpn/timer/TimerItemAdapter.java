/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.timer;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.List;

public class TimerItemAdapter extends RecyclerView.Adapter<TimerItemViewHolder> {
    private final List<TimerItemModel> mTimerItemModels;

    TimerItemAdapter(List<TimerItemModel> timerItemModels) {
        mTimerItemModels = timerItemModels;
    }

    @NonNull
    @Override
    public TimerItemViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View timerItem = layoutInflater.inflate(
                R.layout.fragment_timer_dialog_list_dialog_item, parent, false);
        return new TimerItemViewHolder(timerItem);
    }

    @Override
    public void onBindViewHolder(TimerItemViewHolder holder, int position) {
        TimerItemModel timerItemModel = mTimerItemModels.get(position);
        holder.timerActionText.setText(timerItemModel.getActionText());
        holder.timerActionImage.setImageResource(timerItemModel.getActionImage());
        holder.itemView.setOnClickListener(view
                -> Toast.makeText(view.getContext(), timerItemModel.getActionText(),
                                Toast.LENGTH_SHORT)
                           .show());
    }

    @Override
    public int getItemCount() {
        return mTimerItemModels.size();
    }
}
