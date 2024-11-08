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
import org.chromium.mojo_base.mojom.Value;

import java.util.ArrayList;
import java.util.Map;

public class ContentFilteringAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private static final int TYPE_FILTER_HEADER = 1;
    private static final int TYPE_CUSTOM_FILTER_LIST = 2;
    private static final int TYPE_FILTER_LIST = 3;

    private static final int ONE_ITEM_SPACE = 1;
    private static final int TWO_ITEMS_SPACE = 2;
    private static final int THREE_ITEMS_SPACE = 3;
    private static final int FOUR_ITEMS_SPACE = 4;

    private BraveContentFilteringListener mBraveContentFileringListener;
    private ArrayList<SubscriptionInfo> mSubscriptionFilterLists;
    private Value mFilterLists[];
    private Context mContext;
    private boolean mIsEdit;

    public ContentFilteringAdapter(
            Context context, BraveContentFilteringListener braveContentFileringListener) {
        mContext = context;
        mBraveContentFileringListener = braveContentFileringListener;
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        if (holder instanceof FilterListHeaderViewHolder) {
            FilterListHeaderViewHolder filterListHeaderViewHolder =
                    (FilterListHeaderViewHolder) holder;
            if (holder.getAdapterPosition() == 0) {
                filterListHeaderViewHolder.titleText.setText(R.string.custom_filter_lists);
                filterListHeaderViewHolder.summaryText.setVisibility(View.GONE);
            } else {
                filterListHeaderViewHolder.titleText.setText(R.string.filter_lists);
                filterListHeaderViewHolder.summaryText.setText(R.string.filter_lists_summary);
                filterListHeaderViewHolder.summaryText.setVisibility(View.VISIBLE);
            }
        } else if (holder instanceof CustomFilterListViewHolder) {
            CustomFilterListViewHolder customFilterListViewHolder =
                    (CustomFilterListViewHolder) holder;

            if (holder.getAdapterPosition() == ONE_ITEM_SPACE
                    || holder.getAdapterPosition()
                            == mSubscriptionFilterLists.size() + TWO_ITEMS_SPACE) {
                if (holder.getAdapterPosition() == ONE_ITEM_SPACE) {
                    customFilterListViewHolder.titleText.setText(
                            R.string.create_custom_filters_title);
                } else {
                    customFilterListViewHolder.titleText.setText(R.string.add_custom_filter_list);
                }
                customFilterListViewHolder.lastUpdateText.setVisibility(View.GONE);
                customFilterListViewHolder.toggleSwitch.setVisibility(View.GONE);
                customFilterListViewHolder.urlText.setVisibility(View.GONE);
                customFilterListViewHolder.deleteImageView.setVisibility(View.GONE);
                customFilterListViewHolder.arrowImageView.setVisibility(View.VISIBLE);

            } else {
                SubscriptionInfo customFilter =
                        mSubscriptionFilterLists.get(holder.getAdapterPosition() - TWO_ITEMS_SPACE);
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
                        view -> {
                            customFilter.enabled = !customFilter.enabled;
                            mBraveContentFileringListener.onSubscriptionFilterToggle(
                                    holder.getAdapterPosition() - TWO_ITEMS_SPACE,
                                    customFilter.enabled);
                        });

                if (mIsEdit) {
                    customFilterListViewHolder.deleteImageView.setVisibility(View.VISIBLE);
                    customFilterListViewHolder.toggleSwitch.setVisibility(View.GONE);
                } else {
                    customFilterListViewHolder.deleteImageView.setVisibility(View.GONE);
                    customFilterListViewHolder.toggleSwitch.setVisibility(View.VISIBLE);
                }

                customFilterListViewHolder.deleteImageView.setOnClickListener(
                        view -> {
                            if (mIsEdit) {
                                mBraveContentFileringListener.onSubscriptionFilterDelete(
                                        holder.getAdapterPosition() - TWO_ITEMS_SPACE);
                            }
                        });
                customFilterListViewHolder.urlText.setVisibility(View.VISIBLE);
                customFilterListViewHolder.arrowImageView.setVisibility(View.GONE);
            }

            customFilterListViewHolder.itemView.setOnClickListener(
                    view -> {
                        if (holder.getAdapterPosition() == ONE_ITEM_SPACE) {
                            mBraveContentFileringListener.onCustomFilters();
                        } else if (holder.getAdapterPosition()
                                == mSubscriptionFilterLists.size() + TWO_ITEMS_SPACE) {
                            mBraveContentFileringListener.onAddSubscriptionFilter();
                        }
                    });
        } else if (holder instanceof FilterListViewHolder) {
            FilterListViewHolder filterListViewHolder = (FilterListViewHolder) holder;
            int filterPosition = position - mSubscriptionFilterLists.size() - FOUR_ITEMS_SPACE;
            if (filterPosition < mFilterLists.length) {
                Map<String, Value> storage =
                        mFilterLists[filterPosition].getDictionaryValue().storage;
                String title = storage.get("title").getStringValue();
                String description = storage.get("desc").getStringValue();
                boolean isEnabled = storage.get("enabled").getBoolValue();
                String uuid = storage.get("uuid").getStringValue();
                filterListViewHolder.titleText.setText(title);
                filterListViewHolder.descriptionText.setText(description);
                filterListViewHolder.toggleSwitch.setChecked(isEnabled);

                filterListViewHolder.toggleSwitch.setOnClickListener(
                        view -> {
                            storage.get("enabled").setBoolValue(!isEnabled);
                            mBraveContentFileringListener.onFilterToggle(uuid, !isEnabled);
                        });
            }
        }
    }

    @Override
    public int getItemCount() {
        int count = mSubscriptionFilterLists.size() + FOUR_ITEMS_SPACE;
        if (mFilterLists != null) {
            count += mFilterLists.length;
        }
        return count;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view;

        if (viewType == TYPE_FILTER_HEADER) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.item_filter_title, parent, false);
            return new FilterListHeaderViewHolder(view);
        } else if (viewType == TYPE_CUSTOM_FILTER_LIST) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.item_custom_filter, parent, false);
            return new CustomFilterListViewHolder(view);
        } else {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.item_filter_list, parent, false);
            return new FilterListViewHolder(view);
        }
    }

    @Override
    public int getItemViewType(int position) {
        if (position == 0 || position == mSubscriptionFilterLists.size() + THREE_ITEMS_SPACE) {
            return TYPE_FILTER_HEADER;
        } else if (position > 0 && position <= mSubscriptionFilterLists.size() + TWO_ITEMS_SPACE) {
            return TYPE_CUSTOM_FILTER_LIST;
        } else {
            return TYPE_FILTER_LIST;
        }
    }

    public void setEditable(boolean isEdit) {
        mIsEdit = isEdit;
        notifyItemRangeChanged(TWO_ITEMS_SPACE, mSubscriptionFilterLists.size());
    }

    public void setSubscriptionFilterLists(ArrayList<SubscriptionInfo> customFilterLists) {
        if (mSubscriptionFilterLists != null && mSubscriptionFilterLists.size() > 0) {
            notifyItemRangeRemoved(TWO_ITEMS_SPACE, mSubscriptionFilterLists.size());
        }
        mSubscriptionFilterLists = customFilterLists;
        notifyItemRangeInserted(TWO_ITEMS_SPACE, mSubscriptionFilterLists.size());
    }

    public void setFilterLists(Value filterLists[]) {
        mFilterLists = filterLists;
        // mSubscriptionFilterLists could be null if setSubscriptionFilterLists
        // hasn't been called yet. notifyItemRangeInserted is called inside
        // setSubscriptionFilterLists, so we are good to skip it in that place
        if (mSubscriptionFilterLists != null) {
            notifyItemRangeInserted(
                    mSubscriptionFilterLists.size() + THREE_ITEMS_SPACE,
                    mFilterLists.length + ONE_ITEM_SPACE);
        }
    }

    public static class FilterListHeaderViewHolder extends RecyclerView.ViewHolder {
        TextView titleText;
        TextView summaryText;

        FilterListHeaderViewHolder(View itemView) {
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

    public static class FilterListViewHolder extends RecyclerView.ViewHolder {
        TextView titleText;
        TextView descriptionText;
        SwitchCompat toggleSwitch;

        FilterListViewHolder(View itemView) {
            super(itemView);
            this.titleText = (TextView) itemView.findViewById(R.id.title_text);
            this.descriptionText = (TextView) itemView.findViewById(R.id.description_text);
            this.toggleSwitch = (SwitchCompat) itemView.findViewById(R.id.toggle_switch);
        }
    }
}
