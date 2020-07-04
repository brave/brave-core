/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.brave_stats;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

public class TrackersAdapter extends
    RecyclerView.Adapter<TrackersAdapter.ViewHolder> {

    private int mType;

    public TrackersAdapter (int type) {
        mType = type;
    }

    // Usually involves inflating a layout from XML and returning the holder
    @Override
    public TrackersAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        Context context = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(context);

        // Inflate the custom layout
        View trackerView = inflater.inflate(R.layout.tracker_item_layout, parent, false);

        // Return a new holder instance
        ViewHolder viewHolder = new ViewHolder(trackerView);
        return viewHolder;
    }

    // Involves populating data into the item through holder
    @Override
    public void onBindViewHolder(TrackersAdapter.ViewHolder holder, int position) {
        if (mType == 0) {
            holder.mWebsiteIcon.setVisibility(View.VISIBLE);
        } else if (mType == 1) {
            holder.mTrackerCountText.setVisibility(View.VISIBLE);
        }
    }

    // Returns the total count of items in the list
    @Override
    public int getItemCount() {
        return 100;
    }

    public class ViewHolder extends RecyclerView.ViewHolder {
        // Your holder should contain a member variable
        // for any view that will be set as you render a row
        public TextView mTrackerCountText;
        public ImageView mWebsiteIcon;

        // We also create a constructor that accepts the entire item row
        // and does the view lookups to find each subview
        public ViewHolder(View itemView) {
            // Stores the itemView in a public final member variable that can be used
            // to access the context from any ViewHolder instance.
            super(itemView);

            mTrackerCountText = (TextView) itemView.findViewById(R.id.tracker_count_text);
            mWebsiteIcon = (ImageView) itemView.findViewById(R.id.website_icon);
        }
    }
}