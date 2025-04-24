/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.custom_search_engines.CustomSearchEnginesPrefManager;
import org.chromium.brave.browser.custom_search_engines.R;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.List;

public class CustomSearchEnginesPreference extends Preference {
    private RecyclerView mRecyclerView;
    private Profile mProfile;
    private CustomSearchEngineAdapter mCustomSearchEngineAdapter;
    private CustomSearchEnginesPrefManager mCustomSearchEnginesPrefManager;

    public CustomSearchEnginesPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.custom_search_engines_preference_layout);
    }

    public CustomSearchEnginesPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void initialize(Profile profile) {
        mProfile = profile;
        mCustomSearchEnginesPrefManager = CustomSearchEnginesPrefManager.getInstance();
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.custom_search_engine_list);

        if (mRecyclerView != null) {
            mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        }
        mCustomSearchEngineAdapter = new CustomSearchEngineAdapter(getContext(), mProfile);
        mRecyclerView.setAdapter(mCustomSearchEngineAdapter);

        updateCustomSearchEngines();
    }

    public void updateCustomSearchEngines() {
        if (mRecyclerView == null || mCustomSearchEngineAdapter == null) {
            return;
        }
        List<String> customSearchEngines = mCustomSearchEnginesPrefManager.getCustomSearchEngines();
        mCustomSearchEngineAdapter.submitList(customSearchEngines);
    }
}
