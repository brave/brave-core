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
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.quick_search_engines.ItemTouchHelperCallback;

import java.util.Collections;
import java.util.List;

public class QuickSearchAdapter extends RecyclerView.Adapter<QuickSearchViewHolder> {
    private Context mContext;
    private List<QuickSearchEngineModel> mSearchEngines;
    private QuickSearchCallback mQuickSearchCallback;
    private ItemTouchHelperCallback.OnStartDragListener mDragStartListener;
    private boolean mIsEditMode;

    public QuickSearchAdapter(
            Context context,
            List<QuickSearchEngineModel> searchEngines,
            QuickSearchCallback quickSearchCallback,
            ItemTouchHelperCallback.OnStartDragListener dragStartListener) {
        mContext = context;
        mSearchEngines = searchEngines;
        mQuickSearchCallback = quickSearchCallback;
        mDragStartListener = dragStartListener;
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
                        onSearchEngineClick(
                                quickSearchViewHolder.getAdapterPosition(), quickSearchEngineModel);
                    }
                });
        if (BraveActivity.BING_SEARCH_ENGINE_KEYWORD.equals(quickSearchEngineModel.getKeyword())) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_microsoft_color);
        } else if (BraveActivity.STARTPAGE_SEARCH_ENGINE_KEYWORD.equals(
                quickSearchEngineModel.getKeyword())) {
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
                        if (!mIsEditMode) {
                            setEditMode(true);
                            mQuickSearchCallback.onSearchEngineLongClick();
                        }
                        return true;
                    }
                });
        quickSearchViewHolder.mDragIcon.setOnTouchListener(
                (v, event) -> {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        mDragStartListener.onStartDrag(quickSearchViewHolder);
                    }
                    return false;
                });

        quickSearchViewHolder.mDragIcon.setVisibility(mIsEditMode ? View.VISIBLE : View.GONE);
        quickSearchViewHolder.mSearchEngineSwitch.setVisibility(
                mIsEditMode ? View.GONE : View.VISIBLE);
    }

    private void onSearchEngineClick(int position, QuickSearchEngineModel quickSearchEngineModel) {
        mQuickSearchCallback.onSearchEngineClick(position, quickSearchEngineModel);
    }

    public void swapItems(int fromPosition, int toPosition) {
        Collections.swap(mSearchEngines, fromPosition, toPosition);
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
