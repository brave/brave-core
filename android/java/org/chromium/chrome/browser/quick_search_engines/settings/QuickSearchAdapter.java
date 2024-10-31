/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.Collections;
import java.util.List;

public class QuickSearchAdapter extends RecyclerView.Adapter<QuickSearchViewHolder> {
    private Context mContext;
    private List<QuickSearchEngineModel> mSearchEngines;
    private QuickSearchCallback mQuickSearchCallback;
    private boolean mIsEditMode;

    public QuickSearchAdapter(
            Context context,
            List<QuickSearchEngineModel> searchEngines,
            QuickSearchCallback quickSearchCallback) {
        mContext = context;
        mSearchEngines = searchEngines;
        mQuickSearchCallback = quickSearchCallback;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onBindViewHolder(
            @NonNull QuickSearchViewHolder quickSearchViewHolder, int position) {
        QuickSearchEngineModel quickSearchEngineModel = mSearchEngines.get(position);
        quickSearchViewHolder.mSearchEngineText.setText(quickSearchEngineModel.getShortName());
        quickSearchViewHolder.mSearchEngineSwitch.setChecked(quickSearchEngineModel.isEnabled());
        quickSearchViewHolder.mSearchEngineSwitch.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        quickSearchEngineModel.setEnabled(isChecked);
                        onSearchEngineClick(quickSearchEngineModel);
                    }
                });
        if (quickSearchEngineModel.getKeyword().equals(":b")) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_microsoft_color);
        } else if (quickSearchEngineModel.getKeyword().equals(":sp")) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_startpage_color);
        } else {
            mQuickSearchCallback.loadSearchEngineLogo(
                    quickSearchViewHolder.mSearchEngineLogo, quickSearchEngineModel);
        }

        quickSearchViewHolder.mView.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean isChecked = quickSearchViewHolder.mSearchEngineSwitch.isChecked();
                        quickSearchViewHolder.mSearchEngineSwitch.setChecked(!isChecked);
                    }
                });

        quickSearchViewHolder.mView.setOnLongClickListener(
                new View.OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View v) {
                        setEditMode(true);
                        mQuickSearchCallback.onSearchEngineLongClick();
                        return true;
                    }
                });
        quickSearchViewHolder.mDragIcon.setOnTouchListener(
                new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        if (event.getAction() == MotionEvent.ACTION_DOWN) {
                            ItemTouchHelper itemTouchHelper =
                                    new ItemTouchHelper(
                                            new ItemTouchHelper.SimpleCallback(
                                                    ItemTouchHelper.UP | ItemTouchHelper.DOWN, 0) {
                                                @Override
                                                public boolean onMove(
                                                        @NonNull RecyclerView recyclerView,
                                                        @NonNull RecyclerView.ViewHolder viewHolder,
                                                        @NonNull RecyclerView.ViewHolder target) {
                                                    int fromPosition =
                                                            viewHolder.getAdapterPosition();
                                                    int toPosition = target.getAdapterPosition();
                                                    onItemMove(fromPosition, toPosition);
                                                    return true;
                                                }

                                                @Override
                                                public void onSwiped(
                                                        @NonNull RecyclerView.ViewHolder viewHolder,
                                                        int direction) {
                                                    // No swipe action needed for reordering
                                                }
                                            });
                            itemTouchHelper.attachToRecyclerView(
                                    (RecyclerView) quickSearchViewHolder.itemView.getParent());
                            itemTouchHelper.startDrag(quickSearchViewHolder);
                        }
                        return false;
                    }
                });

        quickSearchViewHolder.mDragIcon.setVisibility(mIsEditMode ? View.VISIBLE : View.GONE);
        quickSearchViewHolder.mSearchEngineSwitch.setVisibility(
                mIsEditMode ? View.GONE : View.VISIBLE);
    }

    private void onSearchEngineClick(QuickSearchEngineModel quickSearchEngineModel) {
        mQuickSearchCallback.onSearchEngineClick(quickSearchEngineModel);
    }

    public void onItemMove(int fromPosition, int toPosition) {
        if (fromPosition < toPosition) {
            for (int i = fromPosition; i < toPosition; i++) {
                Collections.swap(mSearchEngines, i, i + 1);
            }
        } else {
            for (int i = fromPosition; i > toPosition; i--) {
                Collections.swap(mSearchEngines, i, i - 1);
            }
        }
        notifyItemMoved(fromPosition, toPosition);
    }

    @NonNull
    @Override
    public QuickSearchViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.quick_search_settings_item, parent, false);
        return new QuickSearchViewHolder(view);
    }

    @Override
    public int getItemCount() {
        return mSearchEngines.size();
    }

    public List<QuickSearchEngineModel> getSearchEngines() {
        return mSearchEngines;
    }

    public void setEditMode(boolean isEditMode) {
        this.mIsEditMode = isEditMode;
        notifyItemRangeChanged(0, getItemCount());
    }

    public boolean isEditMode() {
        return this.mIsEditMode;
    }

    public List<QuickSearchEngineModel> getQuickSearchEngines() {
        return this.mSearchEngines;
    }
}
