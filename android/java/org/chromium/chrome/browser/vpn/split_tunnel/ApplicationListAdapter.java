/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.split_tunnel;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SortedList;

import org.chromium.chrome.R;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class ApplicationListAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private final SortedList<ApplicationDataModel> mApplicationList;
    private final OnApplicationClickListener mOnApplicationClickListener;
    private final boolean mIsExcludedApps;
    private static final int ANIMATION_DURATION = 1000;

    public Set<String> getApplicationPackages() {
        Set<String> applicationSet = new HashSet<>();
        for (int i = 0; i < mApplicationList.size(); i++) {
            applicationSet.add(mApplicationList.get(i).getPackageName());
        }
        return applicationSet;
    }

    public ApplicationListAdapter(
            OnApplicationClickListener onApplicationClickListener, boolean isExcludedApps) {
        mApplicationList = new SortedList<>(
                ApplicationDataModel.class, new SortedList.Callback<ApplicationDataModel>() {
                    @Override
                    public int compare(ApplicationDataModel applicationDataModel1,
                            ApplicationDataModel applicationDataModel2) {
                        return applicationDataModel1.getName().compareTo(
                                applicationDataModel2.getName());
                    }

                    @Override
                    public void onChanged(int position, int count) {
                        notifyItemRangeChanged(position, count);
                    }

                    @Override
                    public boolean areContentsTheSame(
                            ApplicationDataModel oldItem, ApplicationDataModel newItem) {
                        return oldItem.getName().equals(newItem.getName());
                    }

                    @Override
                    public boolean areItemsTheSame(
                            ApplicationDataModel item1, ApplicationDataModel item2) {
                        return item1.getName().equals(item2.getName());
                    }

                    @Override
                    public void onInserted(int position, int count) {
                        notifyItemRangeInserted(position, count);
                    }

                    @Override
                    public void onRemoved(int position, int count) {
                        notifyItemRangeRemoved(position, count);
                    }

                    @Override
                    public void onMoved(int fromPosition, int toPosition) {
                        notifyItemMoved(fromPosition, toPosition);
                    }
                });
        this.mOnApplicationClickListener = onApplicationClickListener;
        this.mIsExcludedApps = isExcludedApps;
    }

    public void addAll(List<ApplicationDataModel> items) {
        mApplicationList.beginBatchedUpdates();
        for (ApplicationDataModel item : items) {
            mApplicationList.add(item);
        }
        mApplicationList.endBatchedUpdates();
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View rootView = LayoutInflater.from(parent.getContext())
                                .inflate(R.layout.application_item_layout, parent, false);
        return new ApplicationDataViewHolder(rootView);
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        ApplicationDataModel applicationDataModel = mApplicationList.get(position);
        ApplicationDataViewHolder viewHolder = (ApplicationDataViewHolder) holder;

        viewHolder.mAppName.setText(applicationDataModel.getName());
        viewHolder.mAppIcon.setImageDrawable(applicationDataModel.getIcon());
        viewHolder.mActionIcon.setImageResource(
                mIsExcludedApps ? R.drawable.ic_baseline_close_24 : R.drawable.ic_baseline_add_24);
        viewHolder.mActionIcon.setOnClickListener(view
                -> mOnApplicationClickListener.onApplicationCLick(
                        applicationDataModel, position, mIsExcludedApps));
        setFadeAnimation(viewHolder.itemView);
    }

    private void setFadeAnimation(View view) {
        AlphaAnimation anim = new AlphaAnimation(0.0f, 1.0f);
        anim.setDuration(ANIMATION_DURATION);
        view.startAnimation(anim);
    }

    @Override
    public int getItemCount() {
        return mApplicationList.size();
    }

    public void removeApplication(ApplicationDataModel applicationDataModel) {
        mApplicationList.remove(applicationDataModel);
    }

    public void addApplication(ApplicationDataModel applicationDataModel) {
        mApplicationList.add(applicationDataModel);
    }

    static class ApplicationDataViewHolder extends RecyclerView.ViewHolder {
        ImageView mAppIcon;
        TextView mAppName;
        ImageView mActionIcon;

        public ApplicationDataViewHolder(@NonNull View itemView) {
            super(itemView);
            mAppIcon = itemView.findViewById(R.id.app_icon);
            mAppName = itemView.findViewById(R.id.app_name);
            mActionIcon = itemView.findViewById(R.id.action_icon);
        }
    }

    interface OnApplicationClickListener {
        void onApplicationCLick(
                ApplicationDataModel applicationDataModel, int position, boolean isExcludeApps);
    }
}
