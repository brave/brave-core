/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;

import org.chromium.chrome.browser.search_engines.R;

public class CustomSearchEngineAdapter extends ListAdapter<String, CustomSearchEngineViewHolder> {
    private CustomSearchEnginesCallback mCustomSearchEnginesCallback;

    protected CustomSearchEngineAdapter() {
        super(DIFF_CALLBACK);
    }

    private static final DiffUtil.ItemCallback<String> DIFF_CALLBACK =
            new DiffUtil.ItemCallback<String>() {
                @Override
                public boolean areItemsTheSame(@NonNull String oldItem, @NonNull String newItem) {
                    return oldItem.equals(newItem);
                }

                @Override
                public boolean areContentsTheSame(
                        @NonNull String oldItem, @NonNull String newItem) {
                    return oldItem.equals(newItem);
                }
            };

    public void setCustomSearchEnginesCallback(
            CustomSearchEnginesCallback customSearchEnginesCallback) {
        mCustomSearchEnginesCallback = customSearchEnginesCallback;
    }

    @Override
    public void onBindViewHolder(
            @NonNull CustomSearchEngineViewHolder customSearchEngineViewHolder, int position) {
        final String searchEngineKeyword = getItem(position);

        customSearchEngineViewHolder.getSearchEngineText().setText(searchEngineKeyword);

        customSearchEngineViewHolder
                .getView()
                .setOnClickListener(
                        v -> mCustomSearchEnginesCallback.onSearchEngineClick(searchEngineKeyword));
        customSearchEngineViewHolder
                .getDeleteIcon()
                .setOnClickListener(
                        v -> mCustomSearchEnginesCallback.removeSearchEngine(searchEngineKeyword));

        mCustomSearchEnginesCallback.loadSearchEngineLogo(
                customSearchEngineViewHolder.getSearchEngineLogo(), searchEngineKeyword);
    }

    @NonNull
    @Override
    public CustomSearchEngineViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.custom_search_engine_item, parent, false);
        return new CustomSearchEngineViewHolder(view);
    }
}
