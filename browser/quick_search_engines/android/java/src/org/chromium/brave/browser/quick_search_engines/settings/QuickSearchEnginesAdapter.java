/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.settings;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.quick_search_engines.R;
import org.chromium.brave.browser.quick_search_engines.utils.QuickSearchEnginesUtil;

import java.util.List;

public class QuickSearchEnginesAdapter
        extends RecyclerView.Adapter<QuickSearchEnginesSettingsViewHolder> {
    private final List<QuickSearchEnginesModel> mSearchEngines;
    private final QuickSearchEnginesCallback mQuickSearchEnginesCallback;

    public QuickSearchEnginesAdapter(
            List<QuickSearchEnginesModel> searchEngines,
            QuickSearchEnginesCallback quickSearchEnginesCallback) {
        mSearchEngines = searchEngines;
        mQuickSearchEnginesCallback = quickSearchEnginesCallback;
    }

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
            case null, default ->
                    mQuickSearchEnginesCallback.loadSearchEngineLogo(
                            quickSearchEnginesSettingsViewHolder.mSearchEngineLogo,
                            quickSearchEnginesModel);
        }

        quickSearchEnginesSettingsViewHolder.mView.setOnClickListener(
                v -> {
                    boolean isChecked =
                            quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.isChecked();
                    quickSearchEnginesSettingsViewHolder.mSearchEngineSwitch.setChecked(!isChecked);
                });
    }

    private void onSearchEngineClick(
            int position, QuickSearchEnginesModel quickSearchEnginesModel) {
        mQuickSearchEnginesCallback.onSearchEngineClick(position, quickSearchEnginesModel);
    }

    /** Moves an engine within the list while a drag is in progress. */
    public void moveItem(int fromPosition, int toPosition) {
        mSearchEngines.add(toPosition, mSearchEngines.remove(fromPosition));
        notifyItemMoved(fromPosition, toPosition);
    }

    /** Called once the dragged engine has been dropped in its new position. */
    public void onOrderChanged() {
        mQuickSearchEnginesCallback.onSearchEnginesReordered(mSearchEngines);
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
}
