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

public class TwoLineItemRecyclerViewAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private final List<TwoLineItem> mValues;

    public TwoLineItemRecyclerViewAdapter(List<TwoLineItem> items) {
        mValues = items;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        if (viewType == TwoLineItem.TYPE_DIVIDER) {
            return new ViewHolderDivider(
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.item_fragment_two_line_divider_item, parent, false));
        }
        return new ViewHolder(
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.item_fragment_two_line_item, parent, false));
    }

    @Override
    public void onBindViewHolder(final RecyclerView.ViewHolder holder, int position) {
        TwoLineItem twoLineItem = mValues.get(position);
        if (twoLineItem.getType() == TwoLineItem.TYPE_TEXT) {
            ViewHolder viewHolder = (ViewHolder) holder;
            viewHolder.mTvSubtitle.setVisibility(View.VISIBLE);
            viewHolder.mItem = twoLineItem;
            TwoLineItemText itemDataSourceText = (TwoLineItemText) twoLineItem;
            if (itemDataSourceText.getTitle() == null) {
                viewHolder.mTvTitle.setVisibility(View.GONE);
            } else {
                viewHolder.mTvTitle.setText(itemDataSourceText.getTitle());
            }
            if (itemDataSourceText.getSubTitle() == null) {
                viewHolder.mTvSubtitle.setVisibility(View.GONE);
            } else {
                viewHolder.mTvSubtitle.setText(itemDataSourceText.getSubTitle());
            }
        } else if (twoLineItem.getType() == TwoLineItem.TYPE_HEADER) {
            ViewHolder viewHolder = (ViewHolder) holder;
            TwoLineItemHeader itemDataSourceHeader = (TwoLineItemHeader) twoLineItem;
            viewHolder.mTvTitle.setText(itemDataSourceHeader.mHeader);
            viewHolder.mTvSubtitle.setVisibility(View.GONE);
        }
    }

    @Override
    public int getItemViewType(int position) {
        return mValues.get(position).getType();
    }

    @Override
    public int getItemCount() {
        return mValues.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView mTvTitle;
        public final TextView mTvSubtitle;
        public TwoLineItem mItem;

        public ViewHolder(View itemView) {
            super(itemView);
            mTvTitle = itemView.findViewById(R.id.item_fragment_two_line_title);
            mTvSubtitle = itemView.findViewById(R.id.item_fragment_two_line_sub_title);
        }
    }

    public static class ViewHolderDivider extends RecyclerView.ViewHolder {
        public ViewHolderDivider(@NonNull View itemView) {
            super(itemView);
        }
    }
    public interface TwoLineItem {
        int TYPE_TEXT = 1;
        int TYPE_HEADER = 2;
        int TYPE_DIVIDER = 3;

        int getType();
    }

    public static class TwoLineItemText implements TwoLineItem {
        private String title;
        private String subTitle;

        public TwoLineItemText(String title, String subTitle) {
            this.title = title;
            this.subTitle = subTitle;
        }

        public TwoLineItemText(String title) {
            this.title = title;
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

        @Override
        public int getType() {
            return TYPE_TEXT;
        }
    }

    public static class TwoLineItemDivider implements TwoLineItem {
        @Override
        public int getType() {
            return TYPE_DIVIDER;
        }
    }

    public static class TwoLineItemHeader implements TwoLineItem {
        public String mHeader;

        public TwoLineItemHeader(String mHeader) {
            this.mHeader = mHeader;
        }

        @Override
        public int getType() {
            return TYPE_HEADER;
        }
    }
}
