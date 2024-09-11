/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Callbacks;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.base.ViewUtils;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TwoLineItemRecyclerViewAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private List<TwoLineItem> mValues;
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private ADAPTER_VIEW_ORIENTATION mItemViewOrientation;
    private LayoutInflater mLayoutInflater;
    public int mSubTextAlignment;
    public int mDividerMargin;

    public TwoLineItemRecyclerViewAdapter(
            List<TwoLineItem> items, ADAPTER_VIEW_ORIENTATION orientation) {
        mValues = items;
        mItemViewOrientation = orientation;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mDividerMargin = Integer.MIN_VALUE;
    }

    public TwoLineItemRecyclerViewAdapter(List<TwoLineItem> items) {
        this(items, ADAPTER_VIEW_ORIENTATION.VERTICAL);
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        if (viewType == TwoLineItem.TYPE_DIVIDER) {
            View divider =
                    getInflater(parent.getContext())
                            .inflate(R.layout.item_fragment_two_line_divider_item, parent, false);
            if (mDividerMargin != Integer.MIN_VALUE
                    && divider.getLayoutParams() instanceof ViewGroup.MarginLayoutParams) {
                ViewGroup.MarginLayoutParams p =
                        (ViewGroup.MarginLayoutParams) divider.getLayoutParams();
                int pixelMargin = ViewUtils.dpToPx(parent.getContext(), mDividerMargin);
                p.setMargins(0, pixelMargin, 0, pixelMargin);
            }
            return new ViewHolderDivider(divider);
        } else if (viewType == TwoLineItem.TYPE_SINGLE) {
            return new ViewHolderSingleText(
                    getInflater(parent.getContext())
                            .inflate(R.layout.item_fragment_two_line_single_text, parent, false));
        }
        int layout = R.layout.item_fragment_two_line_item;
        if (ADAPTER_VIEW_ORIENTATION.HORIZONTAL == mItemViewOrientation) {
            layout = R.layout.item_two_line_horizontal;
        }
        return new ViewHolder(getInflater(parent.getContext()).inflate(layout, parent, false));
    }

    private LayoutInflater getInflater(Context context) {
        if (mLayoutInflater == null) {
            mLayoutInflater = LayoutInflater.from(context);
        }
        return mLayoutInflater;
    }

    @Override
    public void onBindViewHolder(final RecyclerView.ViewHolder holder, int position) {
        TwoLineItem twoLineItem = mValues.get(position);
        if (twoLineItem.getType() == TwoLineItem.TYPE_TEXT) {
            ViewHolder viewHolder = (ViewHolder) holder;
            // Only vertical layout support image icon and blockies
            if (ADAPTER_VIEW_ORIENTATION.VERTICAL == mItemViewOrientation) {
                AndroidUtils.gone(viewHolder.mIvIconContainer);
            }
            if (mSubTextAlignment != 0) {
                viewHolder.mTvSubtitle.setTextAlignment(mSubTextAlignment);
            }
            viewHolder.mTvSubtitle.setVisibility(View.VISIBLE);
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

            if (ADAPTER_VIEW_ORIENTATION.VERTICAL == mItemViewOrientation
                    && itemDataSourceText.imageType == ImageType.BLOCKIE) {
                AndroidUtils.show(viewHolder.mIvIconContainer);
                Utils.setTextGeneratedBlockies(
                        mExecutor,
                        mHandler,
                        viewHolder.mIvIcon,
                        itemDataSourceText.imgData,
                        true,
                        false);
            }
            if (itemDataSourceText.updateViewCb != null) {
                itemDataSourceText.updateViewCb.call(viewHolder.mTvTitle, viewHolder.mTvSubtitle);
            }
        } else if (twoLineItem.getType() == TwoLineItem.TYPE_HEADER) {
            ViewHolder viewHolder = (ViewHolder) holder;
            if (ADAPTER_VIEW_ORIENTATION.VERTICAL == mItemViewOrientation) {
                AndroidUtils.gone(viewHolder.mIvIconContainer);
            }
            TwoLineItemHeader itemDataSourceHeader = (TwoLineItemHeader) twoLineItem;
            viewHolder.mTvTitle.setText(itemDataSourceHeader.mHeader);
            viewHolder.mTvSubtitle.setVisibility(View.GONE);
        } else if (twoLineItem.getType() == TwoLineItem.TYPE_SINGLE) {
            TwoLineSingleText twoLineSingleItem = (TwoLineSingleText) twoLineItem;
            ViewHolderSingleText viewHolder = (ViewHolderSingleText) holder;
            viewHolder.mTvText.setText(twoLineSingleItem.mText);
            if (twoLineSingleItem.updateViewCb != null) {
                twoLineSingleItem.updateViewCb.call(viewHolder.mTvText);
            }
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

    public void setValues(List<TwoLineItem> items) {
        mValues = items;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final ImageView mIvIcon;
        public final View mIvIconContainer;
        public final TextView mTvTitle;
        public final TextView mTvSubtitle;

        public ViewHolder(View itemView) {
            super(itemView);
            mIvIcon = itemView.findViewById(R.id.item_fragment_two_line_icon);
            mIvIconContainer = itemView.findViewById(R.id.item_fragment_two_line_icon_container);
            mTvTitle = itemView.findViewById(R.id.item_fragment_two_line_title);
            mTvSubtitle = itemView.findViewById(R.id.item_fragment_two_line_sub_title);
        }
    }

    public static class ViewHolderDivider extends RecyclerView.ViewHolder {
        public ViewHolderDivider(@NonNull View itemView) {
            super(itemView);
        }
    }

    public static class ViewHolderSingleText extends RecyclerView.ViewHolder {
        private final TextView mTvText;

        public ViewHolderSingleText(@NonNull View itemView) {
            super(itemView);
            mTvText = itemView.findViewById(R.id.item_fragment_two_line_text);
        }
    }

    public interface TwoLineItem {
        int TYPE_TEXT = 1;
        int TYPE_HEADER = 2;
        int TYPE_DIVIDER = 3;
        int TYPE_SINGLE = 4;

        int getType();
    }

    public static class TwoLineItemText implements TwoLineItem {
        public String title;
        public String subTitle;
        public ImageType imageType;
        public String imgData;

        private Callbacks.Callback2<TextView, TextView> updateViewCb;

        public TwoLineItemText(
                String title,
                String subTitle,
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

    public static class TwoLineSingleText implements TwoLineItem {
        public String mText;
        public Callbacks.Callback1<TextView> updateViewCb;

        public TwoLineSingleText() {
            mText = "";
        }

        @Override
        public int getType() {
            return TYPE_SINGLE;
        }
    }

    public enum ImageType {
        NONE,
        BLOCKIE
    }

    public enum ADAPTER_VIEW_ORIENTATION {
        HORIZONTAL,
        VERTICAL
    }
}
