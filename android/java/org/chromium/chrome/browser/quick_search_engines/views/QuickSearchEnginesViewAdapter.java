/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.views;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchCallback;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEngineModel;

import java.util.List;

public class QuickSearchEnginesViewAdapter
        extends RecyclerView.Adapter<QuickSearchEnginesViewHolder> {
    private Context mContext;
    private List<QuickSearchEngineModel> mSearchEngines;
    private QuickSearchCallback mQuickSearchCallback;

    public QuickSearchEnginesViewAdapter(
            Context context,
            List<QuickSearchEngineModel> searchEngines,
            QuickSearchCallback quickSearchCallback) {
        mContext = context;
        mSearchEngines = searchEngines;
        mQuickSearchCallback = quickSearchCallback;
    }

    @Override
    public void onBindViewHolder(
            @NonNull QuickSearchEnginesViewHolder quickSearchViewHolder, int position) {
        QuickSearchEngineModel quickSearchEngineModel = mSearchEngines.get(position);
        int adapterPosition = quickSearchViewHolder.getAdapterPosition();
        String keyword = quickSearchEngineModel.getKeyword();

        if (adapterPosition == 0) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_leo_icon);
        } else if (BraveActivity.BING_SEARCH_ENGINE_KEYWORD.equals(keyword)) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_microsoft_color);
        } else if (BraveActivity.STARTPAGE_SEARCH_ENGINE_KEYWORD.equals(keyword)) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_startpage_color);
        } else {
            mQuickSearchCallback.loadSearchEngineLogo(
                    quickSearchViewHolder.mSearchEngineLogo, quickSearchEngineModel);
        }

        quickSearchViewHolder.mSearchEngineLogo.setOnClickListener(
                v ->
                        mQuickSearchCallback.onSearchEngineClick(
                                adapterPosition, quickSearchEngineModel));
    }

    @NonNull
    @Override
    public QuickSearchEnginesViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.quick_search_engines_view_item, parent, false);
        return new QuickSearchEnginesViewHolder(view);
    }

    @Override
    public int getItemCount() {
        return mSearchEngines.size();
    }

    public List<QuickSearchEngineModel> getSearchEngines() {
        return mSearchEngines;
    }
}
