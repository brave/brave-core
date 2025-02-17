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
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;

import java.util.List;

public class CustomSearchEnginesPreference extends Preference
        implements CustomSearchEnginesCallback {
    private RecyclerView mRecyclerView;
    private Profile mProfile;
    private CustomSearchEngineAdapter mCustomSearchEngineAdapter;

    public CustomSearchEnginesPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.custom_search_engines_preference_layout);
    }

    public CustomSearchEnginesPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.e("brave_search", "CustomSearchEnginesPreference");
    }

    public void initialize(Profile profile) {
        Log.e("brave_search", "initialize(Profile profile)");
        mProfile = profile;
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        Log.e("brave_search", "onBindViewHolder");
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.custom_search_engine_list);

        if (mRecyclerView != null) {
            mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        }
        mCustomSearchEngineAdapter = new CustomSearchEngineAdapter();
        mCustomSearchEngineAdapter.setCustomSearchEnginesCallback(this);
        mRecyclerView.setAdapter(mCustomSearchEngineAdapter);

        updateCustomSearchEngines();
    }

    public void updateCustomSearchEngines() {
        if (mRecyclerView == null || mCustomSearchEngineAdapter == null) {
            Log.e("brave_search", "updateCustomSearchEngines 1");
            return;
        }
        List<String> customSearchEngines = CustomSearchEnginesUtil.getCustomSearchEngines();
        for (String keyword : customSearchEngines) {
            Log.e("brave_search", "updateCustomSearchEngines : keyword : " + keyword);
        }
        mCustomSearchEngineAdapter.submitList(customSearchEngines);
    }

    @Override
    public void onSearchEngineClick(String searchEngineKeyword) {}

    @Override
    public void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {
        if (mProfile != null) {
            CustomSearchEnginesUtil.loadSearchEngineLogo(mProfile, logoView, searchEngineKeyword);
        }
    }

    @Override
    public void removeSearchEngine(String searchEngineKeyword) {
        Runnable templateUrlServiceReady =
                () -> {
                    Log.e("brave_search", "removeSearchEngine");
                    TemplateUrlServiceFactory.getForProfile(mProfile)
                            .removeSearchEngine(searchEngineKeyword);
                    CustomSearchEnginesUtil.removeCustomSearchEngine(searchEngineKeyword);
                    updateCustomSearchEngines();
                };
        TemplateUrlServiceFactory.getForProfile(mProfile).runWhenLoaded(templateUrlServiceReady);
    }
}
