/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.R;

import java.util.List;

public class CustomSearchEnginesPreference extends Preference
        implements CustomSearchEnginesCallback {
    private RecyclerView mRecyclerView;

    public CustomSearchEnginesPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.e("brave_search", "CustomSearchEnginesPreference");
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        Log.e("brave_search", "onBindViewHolder");
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.custom_search_engine_list);

        if (mRecyclerView != null) {
            mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        }
        updateCustomSearchEngines();
    }

    public void updateCustomSearchEngines() {
        if (mRecyclerView == null) {
            Log.e("brave_search", "updateCustomSearchEngines 1");
            return;
        }
        List<String> customSearchEngines = CustomSearchEnginesUtil.getCustomSearchEngines();
        Log.e("brave_search", "updateCustomSearchEngines");
        final CustomSearchEngineAdapter adapter =
                new CustomSearchEngineAdapter(customSearchEngines, this);
        mRecyclerView.setAdapter(adapter);
    }

    @Override
    public void onSearchEngineClick(String searchEngineKeyword) {}

    @Override
    public void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {
        CustomSearchEnginesUtil.loadSearchEngineLogo(
                ProfileManager.getLastUsedRegularProfile(), logoView, searchEngineKeyword);
    }

    @Override
    public void removeSearchEngine(String searchEngineKeyword) {}
}
