/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.settings;

import android.annotation.SuppressLint;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.quick_search_engines.ItemTouchHelperCallback;
import org.chromium.brave.browser.quick_search_engines.R;
import org.chromium.brave.browser.quick_search_engines.utils.QuickSearchEnginesUtil;

import java.util.Collections;
import java.util.List;

public class QuickSearchEnginesAdapter
        extends RecyclerView.Adapter<QuickSearchEnginesSettingsViewHolder> {
    private final List<QuickSearchEnginesModel> mSearchEngines;
    private final QuickSearchEnginesCallback mQuickSearchEnginesCallback;
    private final ItemTouchHelperCallback.OnStartDragListener mDragStartListener;
    private boolean mIsEditMode;

    public QuickSearchEnginesAdapter(
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
                (buttonView, isChecked) -> {
                    // Update enabled state of search engine at current adapter position
                    int position1 = quickSearchEnginesSettingsViewHolder.getAdapterPosition();
                    QuickSearchEnginesModel searchEngine = mSearchEngines.get(position1);
                    searchEngine.setEnabled(isChecked);
                    onSearchEngineClick(
                            quickSearchEnginesSettingsViewHolder.getAdapterPosition(),
                            quickSearchEnginesModel);
                });
        switch (quickSearchEnginesModel.getKeyword()) {
            case QuickSearchEnginesUtil.YOUTUBE_SEARCH_ENGINE_KEYWORD ->
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                            R.drawable.ic_social_youtube);
            case QuickSearchEnginesUtil.BING_SEARCH_ENGINE_KEYWORD ->
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                            R.drawable.ic_microsoft_color);
            case QuickSearchEnginesUtil.STARTPAGE_SEARCH_ENGINE_KEYWORD ->
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                            R.drawable.ic_startpage_color);
            case QuickSearchEnginesUtil.BRAVE_SEARCH_ENGINE_KEYWORD ->
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo.setImageResource(
                            R.drawable.ic_social_brave_release_favicon_fullheight_color);
            case null, default -> mQuickSearchEnginesCallback.loadSearchEngineLogo(
                    quickSearchEnginesSettingsViewHolder.mSearchEngineLogo,
                    quickSearchEnginesModel);
        }

        quickSearchEnginesSettingsViewHolder.mView.setOnClickListener(
                v -> {
                    boolean isChecked =
                            quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch
                                    .isChecked();
                    quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setChecked(
                            !isChecked);
                });

        quickSearchEnginesSettingsViewHolder.mView.setOnLongClickListener(
                v -> {
                    if (!mIsEditMode) {
                        setEditMode(true);
                        mQuickSearchEnginesCallback.onSearchEngineLongClick();
                    }
                    return true;
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
