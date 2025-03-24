//
/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons.settings;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave.browser.custom_app_icons.CustomAppIconsEnum;
import org.chromium.brave.browser.custom_app_icons.CustomAppIconsManager;
import org.chromium.brave.browser.custom_app_icons.R;

public class CustomAppIconsAdapter extends RecyclerView.Adapter<CustomAppIconsAdapter.ViewHolder> {

    private CustomAppIconsEnum[] mCustomAppIcons;

    public CustomAppIconsAdapter(CustomAppIconsEnum[] customAppIcons) {
        mCustomAppIcons = customAppIcons;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.custom_app_icons_item, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        holder.mTitle.setText(mCustomAppIcons[position].getDesc());
        if (mCustomAppIcons[position].equals(CustomAppIconsEnum.ICON_DEFAULT)) {
            holder.mIcon.setImageResource(R.drawable.ic_launcher);
        } else {
            holder.mIcon.setImageResource(mCustomAppIcons[position].getIcon());
        }

        boolean shouldShowSelected =
                mCustomAppIcons[position].equals(
                        CustomAppIconsManager.getCurrentIcon(holder.mView.getContext()));
        holder.mCheck.setVisibility(shouldShowSelected ? View.VISIBLE : View.GONE);
        holder.mView.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        PostTask.postDelayedTask(
                                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                                () -> {
                                    CustomAppIconsManager.switchIcon(
                                            v.getContext(),
                                            mCustomAppIcons[holder.getAdapterPosition()]);
                                },
                                500);
                    }
                });
    }

    @Override
    public int getItemCount() {
        return mCustomAppIcons.length;
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        TextView mTitle;
        ImageView mIcon;
        ImageView mCheck;
        View mView;

        ViewHolder(View itemView) {
            super(itemView);
            mView = itemView;
            mTitle = itemView.findViewById(R.id.title);
            mIcon = itemView.findViewById(R.id.icon);
            mCheck = itemView.findViewById(R.id.check);
        }
    }
}
