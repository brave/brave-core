/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.List;

public class TwoLineItemRecyclerViewAdapter
        extends RecyclerView.Adapter<TwoLineItemRecyclerViewAdapter.ViewHolder> {
    private final List<TwoLineItemDataSource> mValues;

    public TwoLineItemRecyclerViewAdapter(List<TwoLineItemDataSource> items) {
        mValues = items;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        return new ViewHolder(
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.item_fragment_two_line_item, parent, false));
    }

    @Override
    public void onBindViewHolder(final ViewHolder holder, int position) {
        holder.mItem = mValues.get(position);
        holder.mTvTitle.setText(mValues.get(position).getSubTitle());
        holder.mTvSubtitle.setText(mValues.get(position).getSubTitle());
    }

    @Override
    public int getItemCount() {
        return mValues.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView mTvTitle;
        public final TextView mTvSubtitle;
        public TwoLineItemDataSource mItem;

        public ViewHolder(View itemView) {
            super(itemView);
            mTvTitle = itemView.findViewById(R.id.item_fragment_two_line_title);
            mTvSubtitle = itemView.findViewById(R.id.item_fragment_two_line_sub_title);
        }

        @NonNull
        @Override
        public String toString() {
            return mTvTitle.toString() + " '" + mTvSubtitle.getText() + "'";
        }
    }

    public static class TwoLineItemDataSource {
        private String title;
        private String subTitle;

        public TwoLineItemDataSource(String title, String subTitle) {
            this.title = title;
            this.subTitle = subTitle;
        }

        public String getTitle() {
            return title;
        }

        public void setTitle(String title) {
            this.title = title;
        }

        public String getSubTitle() {
            return subTitle;
        }

        public void setSubTitle(String subTitle) {
            this.subTitle = subTitle;
        }
    }
}
