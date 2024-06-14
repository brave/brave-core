/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;

import java.util.List;

public class QuickSearchAdapter extends RecyclerView.Adapter<QuickSearchViewHolder> {
    private Context mContext;
    private List<QuickSearchEngineModel> mSearchEngines;
    private QuickSearchCallback mQuickSearchCallback;

    public QuickSearchAdapter(
            Context context,
            List<QuickSearchEngineModel> searchEngines,
            QuickSearchCallback quickSearchCallback) {
        mContext = context;
        mSearchEngines = searchEngines;
        mQuickSearchCallback = quickSearchCallback;
    }

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
                        Log.e("quick_search", "isChecked : " + isChecked);
                        onSearchEngineClick(quickSearchEngineModel);
                    }
                });

        mQuickSearchCallback.loadSearchEngineLogo(
                quickSearchViewHolder.mSearchEngineLogo, quickSearchEngineModel);

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
                        mQuickSearchCallback.onSearchEngineLongClick();
                        return true;
                    }
                });
    }

    private void onSearchEngineClick(QuickSearchEngineModel quickSearchEngineModel) {
        mQuickSearchCallback.onSearchEngineClick(quickSearchEngineModel);
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
}
