/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.Callbacks;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TwoLineItemRecyclerViewAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private final List<TwoLineItem> mValues;
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private ORIENTATION mOrientation;

    public TwoLineItemRecyclerViewAdapter(List<TwoLineItem> items, ORIENTATION orientation) {
        mValues = items;
        mOrientation = orientation;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    public TwoLineItemRecyclerViewAdapter(List<TwoLineItem> items) {
        this(items, ORIENTATION.VERTICAL);
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        if (viewType == TwoLineItem.TYPE_DIVIDER) {
            return new ViewHolderDivider(
                    LayoutInflater.from(parent.getContext())
                            .inflate(R.layout.item_fragment_two_line_divider_item, parent, false));
        }
        int layout = R.layout.item_fragment_two_line_item;
        if (ORIENTATION.HORIZONTAL == mOrientation) {
            layout = R.layout.item_two_line_horizontal;
        }
        return new ViewHolder(
                LayoutInflater.from(parent.getContext()).inflate(layout, parent, false));
    }

    @Override
    public void onBindViewHolder(final RecyclerView.ViewHolder holder, int position) {
        TwoLineItem twoLineItem = mValues.get(position);
        ViewHolder viewHolder = (ViewHolder) holder;
        AndroidUtils.gone(viewHolder.mIvIcon);
        if (twoLineItem.getType() == TwoLineItem.TYPE_TEXT) {
            viewHolder.mTvSubtitle.setVisibility(View.VISIBLE);
            viewHolder.mItem = twoLineItem;
            TwoLineItemText itemDataSourceText = (TwoLineItemText) twoLineItem;
            if (itemDataSourceText.title == null) {
                viewHolder.mTvTitle.setVisibility(View.GONE);
            } else {
                viewHolder.mTvTitle.setText(itemDataSourceText.title);
            }
            if (itemDataSourceText.subTitle == null) {
                viewHolder.mTvSubtitle.setVisibility(View.GONE);
            } else {
                viewHolder.mTvSubtitle.setText(itemDataSourceText.subTitle);
            }

            if (itemDataSourceText.imageType == ImageType.BLOCKIE) {
                AndroidUtils.show(viewHolder.mIvIcon);
                Utils.setTextGeneratedBlockies(
                        mExecutor, mHandler, viewHolder.mIvIcon, itemDataSourceText.imgData, true);
            }
            if (itemDataSourceText.updateViewCb != null) {
                itemDataSourceText.updateViewCb.call(viewHolder.mTvTitle, viewHolder.mTvSubtitle);
            }
        } else if (twoLineItem.getType() == TwoLineItem.TYPE_HEADER) {
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
        public ImageView mIvIcon;
        public final TextView mTvTitle;
        public final TextView mTvSubtitle;
        public TwoLineItem mItem;

        public ViewHolder(View itemView) {
            super(itemView);
            mIvIcon = itemView.findViewById(R.id.item_create_account_iv_icon);
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
        public String title;
        public String subTitle;
        public ImageType imageType;
        public String imgData;

        private Callbacks.Callback2<TextView, TextView> updateViewCb;

        public TwoLineItemText(String title, String subTitle,
                Callbacks.Callback2<TextView, TextView> customUiChanges) {
            this(title, subTitle);
            this.updateViewCb = customUiChanges;
        }

        public TwoLineItemText(String title, String subTitle) {
            this.title = title;
            this.subTitle = subTitle;
        }

        public TwoLineItemText(String title) {
            this.title = title;
        }

        public void setTitle(String title) {
            this.title = title;
        }

        @Override
        public int getType() {
            return TYPE_TEXT;
        }

        public ImageType getImageType() {
            return imageType;
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

    public enum ImageType { NONE, BLOCKIE }
    public enum ORIENTATION { HORIZONTAL, VERTICAL }
}
