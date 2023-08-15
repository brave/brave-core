/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.content.Context;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_shields.mojom.SubscriptionInfo;
import org.chromium.chrome.R;

import java.util.ArrayList;

public class ContentFilteringAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private static int TYPE_CUSTOM_FILTER_HEADER = 1;
    private static int TYPE_CUSTOM_FILTER_LIST = 2;

    private BraveContentFilteringListener mBraveContentFileringListener;
    private ArrayList<SubscriptionInfo> mCustomFilterLists;
    private Context mContext;
    private boolean mIsEdit;

    public ContentFilteringAdapter(
            Context context, BraveContentFilteringListener braveContentFileringListener) {
        mContext = context;
        mBraveContentFileringListener = braveContentFileringListener;
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        if (holder instanceof CustomFilterHeaderViewHolder) {
            CustomFilterHeaderViewHolder customFilterHeaderViewHolder =
                    (CustomFilterHeaderViewHolder) holder;
            customFilterHeaderViewHolder.titleText.setText(R.string.custom_filter_lists);
            customFilterHeaderViewHolder.summaryText.setVisibility(View.GONE);
        } else if (holder instanceof CustomFilterListViewHolder) {
            CustomFilterListViewHolder customFilterListViewHolder =
                    (CustomFilterListViewHolder) holder;

            if (holder.getAdapterPosition() == mCustomFilterLists.size() + 1) {
                customFilterListViewHolder.titleText.setText(R.string.add_custom_filter_list);
                customFilterListViewHolder.lastUpdateText.setVisibility(View.GONE);
                customFilterListViewHolder.toggleSwitch.setVisibility(View.GONE);
                customFilterListViewHolder.urlText.setVisibility(View.GONE);
                customFilterListViewHolder.deleteImageView.setVisibility(View.GONE);
                customFilterListViewHolder.arrowImageView.setVisibility(View.VISIBLE);

            } else {
                SubscriptionInfo customFilter =
                        mCustomFilterLists.get(holder.getAdapterPosition() - 1);
                String url = customFilter.subscriptionUrl.url;
                if (customFilter.title != null && customFilter.title.length() > 0) {
                    customFilterListViewHolder.titleText.setText(customFilter.title);
                    customFilterListViewHolder.titleText.setVisibility(View.VISIBLE);

                } else {
                    Uri uri = Uri.parse(url);
                    String lastPathSegment = uri.getLastPathSegment();
                    if (lastPathSegment != null && lastPathSegment.length() > 0) {
                        customFilterListViewHolder.titleText.setText(lastPathSegment);
                        customFilterListViewHolder.titleText.setVisibility(View.VISIBLE);
                    } else if (customFilter.homepage != null
                            && customFilter.homepage.length() > 0) {
                        customFilterListViewHolder.titleText.setText(customFilter.homepage);
                        customFilterListViewHolder.titleText.setVisibility(View.VISIBLE);
                    } else {
                        customFilterListViewHolder.titleText.setText(uri.getHost());
                        customFilterListViewHolder.titleText.setVisibility(View.VISIBLE);
                    }
                }

                if (customFilter.lastUpdateAttempt.internalValue == 0) {
                    customFilterListViewHolder.lastUpdateText.setText(R.string.dashed);
                    customFilterListViewHolder.lastUpdateText.setTextColor(
                            ContextCompat.getColor(mContext, R.color.filter_summary_color));
                    customFilterListViewHolder.lastUpdateText.setVisibility(View.VISIBLE);

                } else if (customFilter.lastSuccessfulUpdateAttempt.internalValue == 0) {
                    customFilterListViewHolder.lastUpdateText.setText(
                            R.string.download_failed_custom_filter);

                    customFilterListViewHolder.lastUpdateText.setTextColor(ContextCompat.getColor(
                            mContext, R.color.add_custom_filter_error_text_color));
                    customFilterListViewHolder.lastUpdateText.setVisibility(View.VISIBLE);

                } else if (customFilter.lastSuccessfulUpdateAttempt.internalValue != 0
                        && customFilter.lastSuccessfulUpdateAttempt.internalValue
                                != customFilter.lastUpdateAttempt.internalValue) {
                    customFilterListViewHolder.lastUpdateText.setText(
                            mContext.getResources().getString(R.string.update_failed_custom_filter,
                                    customFilter.lastUpdatedPrettyText));

                    customFilterListViewHolder.lastUpdateText.setTextColor(
                            ContextCompat.getColor(mContext, R.color.filter_summary_color));
                    customFilterListViewHolder.lastUpdateText.setVisibility(View.VISIBLE);

                } else {
                    customFilterListViewHolder.lastUpdateText.setText(
                            mContext.getResources().getString(R.string.last_updated_custom_filter,
                                    customFilter.lastUpdatedPrettyText));
                    customFilterListViewHolder.lastUpdateText.setTextColor(
                            ContextCompat.getColor(mContext, R.color.filter_summary_color));
                    customFilterListViewHolder.lastUpdateText.setVisibility(View.VISIBLE);
                }

                customFilterListViewHolder.urlText.setText(url);
                customFilterListViewHolder.toggleSwitch.setChecked(customFilter.enabled);

                customFilterListViewHolder.toggleSwitch.setOnClickListener(
                        new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                customFilter.enabled = !customFilter.enabled;
                                mBraveContentFileringListener.onCustomFilterToggle(
                                        holder.getAdapterPosition() - 1, customFilter.enabled);
                            }
                        });

                if (mIsEdit) {
                    customFilterListViewHolder.deleteImageView.setVisibility(View.VISIBLE);
                    customFilterListViewHolder.toggleSwitch.setVisibility(View.GONE);
                } else {
                    customFilterListViewHolder.deleteImageView.setVisibility(View.GONE);
                    customFilterListViewHolder.toggleSwitch.setVisibility(View.VISIBLE);
                }

                customFilterListViewHolder.deleteImageView.setOnClickListener(view -> {
                    if (mIsEdit) {
                        mBraveContentFileringListener.onCustomFilterDelete(
                                holder.getAdapterPosition() - 1);
                    }
                });
                customFilterListViewHolder.urlText.setVisibility(View.VISIBLE);
                customFilterListViewHolder.arrowImageView.setVisibility(View.GONE);
            }

            customFilterListViewHolder.itemView.setOnClickListener(view -> {
                if (holder.getAdapterPosition() == mCustomFilterLists.size() + 1) {
                    mBraveContentFileringListener.onAddCustomFiltering();
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return mCustomFilterLists.size() + 2;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view;
        if (viewType == TYPE_CUSTOM_FILTER_HEADER) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.item_filter_title, parent, false);
            return new CustomFilterHeaderViewHolder(view);
        } else {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.item_custom_filter, parent, false);
            return new CustomFilterListViewHolder(view);
        }
    }

    @Override
    public int getItemViewType(int position) {
        if (position == 0) {
            return TYPE_CUSTOM_FILTER_HEADER;
        } else {
            return TYPE_CUSTOM_FILTER_LIST;
        }
    }

    public void setEditable(boolean isEdit) {
        mIsEdit = isEdit;
        notifyItemRangeChanged(1, mCustomFilterLists.size());
    }

    public void setCustomFilterLists(ArrayList<SubscriptionInfo> customFilterLists) {
        if (mCustomFilterLists != null && mCustomFilterLists.size() > 0) {
            notifyItemRangeRemoved(1, mCustomFilterLists.size());
        }
        mCustomFilterLists = customFilterLists;
        notifyItemRangeInserted(1, mCustomFilterLists.size());
    }

    public static class CustomFilterHeaderViewHolder extends RecyclerView.ViewHolder {
        TextView titleText;
        TextView summaryText;

        CustomFilterHeaderViewHolder(View itemView) {
            super(itemView);
            this.titleText = (TextView) itemView.findViewById(R.id.title_text);
            this.summaryText = (TextView) itemView.findViewById(R.id.summary_text);
        }
    }

    public static class CustomFilterListViewHolder extends RecyclerView.ViewHolder {
        TextView titleText;
        TextView lastUpdateText;
        TextView urlText;
        ImageView deleteImageView;
        ImageView arrowImageView;
        SwitchCompat toggleSwitch;
        View divider;

        CustomFilterListViewHolder(View itemView) {
            super(itemView);
            this.titleText = (TextView) itemView.findViewById(R.id.title_text);
            this.lastUpdateText = (TextView) itemView.findViewById(R.id.last_update_text);
            this.urlText = (TextView) itemView.findViewById(R.id.url_text);
            this.deleteImageView = (ImageView) itemView.findViewById(R.id.iv_delete);
            this.arrowImageView = (ImageView) itemView.findViewById(R.id.iv_arrow);
            this.toggleSwitch = (SwitchCompat) itemView.findViewById(R.id.toggle_switch);
            this.divider = itemView.findViewById(R.id.divider);
        }
    }
}
