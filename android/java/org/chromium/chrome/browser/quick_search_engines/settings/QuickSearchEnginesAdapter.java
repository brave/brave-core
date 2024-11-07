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

public class QuickSearchEnginesAdapter
        extends RecyclerView.Adapter<QuickSearchEnginesSettingsViewHolder> {
    private List<QuickSearchEnginesModel> mSearchEngines;
    private QuickSearchEnginesCallback mQuickSearchEnginesCallback;
    private ItemTouchHelperCallback.OnStartDragListener mDragStartListener;
    private boolean mIsEditMode;

    public QuickSearchEnginesAdapter(
            Context context,
            List<QuickSearchEnginesModel> searchEngines,
            QuickSearchEnginesCallback quickSearchEnginesCallback,
            ItemTouchHelperCallback.OnStartDragListener dragStartListener) {
        mSearchEngines = searchEngines;
        mQuickSearchEnginesCallback = quickSearchEnginesCallback;
        mDragStartListener = dragStartListener;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onBindViewHolder(
            @NonNull QuickSearchEnginesSettingsViewHolder quickSearchEnginesSettingsViewHolder,
            int position) {
        QuickSearchEnginesModel quickSearchEnginesModel = mSearchEngines.get(position);
        quickSearchEnginesSettingsViewHolder.mSearchEngineText.setText(
                quickSearchEnginesModel.getShortName());
        quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setChecked(
                quickSearchEnginesModel.isEnabled());
        quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        quickSearchEnginesModel.setEnabled(isChecked);
                        onSearchEngineClick(
                                quickSearchEnginesSettingsViewHolder.getAdapterPosition(),
                                quickSearchEnginesModel);
                    }
                });
        if (BraveActivity.BING_SEARCH_ENGINE_KEYWORD.equals(quickSearchEnginesModel.getKeyword())) {
            quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                    R.drawable.ic_microsoft_color);
        } else if (BraveActivity.STARTPAGE_SEARCH_ENGINE_KEYWORD.equals(
                quickSearchEnginesModel.getKeyword())) {
            quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                    R.drawable.ic_startpage_color);
        } else {
            mQuickSearchEnginesCallback.loadSearchEngineLogo(
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo,
                    quickSearchEnginesModel);
        }

        quickSearchEnginesSettingsViewHolder.mView.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean isChecked =
                                quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch
                                        .isChecked();
                        quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setChecked(
                                !isChecked);
                    }
                });

        quickSearchEnginesSettingsViewHolder.mView.setOnLongClickListener(
                new View.OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View v) {
                        if (!mIsEditMode) {
                            setEditMode(true);
                            mQuickSearchEnginesCallback.onSearchEngineLongClick();
                        }
                        return true;
                    }
                });
        quickSearchEnginesSettingsViewHolder.mDragIcon.setOnTouchListener(
                (v, event) -> {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        mDragStartListener.onStartDrag(quickSearchEnginesSettingsViewHolder);
                    }
                    return false;
                });

        quickSearchEnginesSettingsViewHolder.mDragIcon.setVisibility(
                mIsEditMode ? View.VISIBLE : View.GONE);
        quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setVisibility(
                mIsEditMode ? View.GONE : View.VISIBLE);
    }

    private void onSearchEngineClick(
            int position, QuickSearchEnginesModel quickSearchEnginesModel) {
        mQuickSearchEnginesCallback.onSearchEngineClick(position, quickSearchEnginesModel);
    }

    public void swapItems(int fromPosition, int toPosition) {
        Collections.swap(mSearchEngines, fromPosition, toPosition);
        notifyItemMoved(fromPosition, toPosition);
    }

    @NonNull
    @Override
    public QuickSearchEnginesSettingsViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.quick_search_settings_item, parent, false);
        return new QuickSearchEnginesSettingsViewHolder(view);
    }

    @Override
    public int getItemCount() {
        return mSearchEngines.size();
    }

    public List<QuickSearchEnginesModel> getSearchEngines() {
        return mSearchEngines;
    }

    public void setEditMode(boolean isEditMode) {
        this.mIsEditMode = isEditMode;
        notifyItemRangeChanged(0, getItemCount());
    }

    public boolean isEditMode() {
        return this.mIsEditMode;
    }

    public List<QuickSearchEnginesModel> getQuickSearchEngines() {
        return this.mSearchEngines;
    }
}
