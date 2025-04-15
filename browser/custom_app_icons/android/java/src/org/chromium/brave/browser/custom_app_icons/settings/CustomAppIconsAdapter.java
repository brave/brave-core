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

import org.chromium.brave.browser.custom_app_icons.CustomAppIcons;
import org.chromium.brave.browser.custom_app_icons.CustomAppIcons.AppIconType;
import org.chromium.brave.browser.custom_app_icons.CustomAppIconsManager;
import org.chromium.brave.browser.custom_app_icons.R;

public class CustomAppIconsAdapter extends RecyclerView.Adapter<CustomAppIconsAdapter.ViewHolder> {
    private final @AppIconType int[] mCustomAppIcons;
    private final CustomAppIconsListener mListener;

    public CustomAppIconsAdapter(
            @AppIconType int[] customAppIcons, CustomAppIconsListener listener) {
        mCustomAppIcons = customAppIcons;
        mListener = listener;
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
        @AppIconType int currentIcon = mCustomAppIcons[position];

        holder.mTitle.setText(CustomAppIcons.getTitle(currentIcon));
        holder.mIcon.setImageResource(
                currentIcon == CustomAppIcons.ICON_DEFAULT
                        ? R.drawable.ic_launcher
                        : CustomAppIcons.getIcon(currentIcon));

        boolean isSelected = currentIcon == CustomAppIconsManager.getInstance().getCurrentIcon();
        holder.mCheck.setVisibility(isSelected ? View.VISIBLE : View.GONE);

        holder.itemView.setOnClickListener(
                v -> {
                    if (mListener != null) {
                        mListener.onCustomAppIconSelected(
                                mCustomAppIcons[holder.getAdapterPosition()]);
                    }
                });
    }

    @Override
    public int getItemCount() {
        return mCustomAppIcons.length;
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        final TextView mTitle;
        final ImageView mIcon;
        final ImageView mCheck;

        ViewHolder(View itemView) {
            super(itemView);
            mTitle = itemView.findViewById(R.id.title);
            mIcon = itemView.findViewById(R.id.icon);
            mCheck = itemView.findViewById(R.id.check);
        }
    }
}
