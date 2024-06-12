/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.components.search_engines.TemplateUrl;

import java.util.List;

public class QuickSearchAdapter extends RecyclerView.Adapter<QuickSearchViewHolder> {
    private Context mContext;
    private List<TemplateUrl> mSearchEngines;

    public QuickSearchAdapter(Context context, List<TemplateUrl> searchEngines) {
        mContext = context;
        mSearchEngines = searchEngines;
    }

    @Override
    public void onBindViewHolder(@NonNull QuickSearchViewHolder holder, int position) {
        TemplateUrl templateUrl = mSearchEngines.get(position);
        Log.e("quick_search", templateUrl.getShortName());
        holder.searchEngineText.setText(templateUrl.getShortName());
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
}
