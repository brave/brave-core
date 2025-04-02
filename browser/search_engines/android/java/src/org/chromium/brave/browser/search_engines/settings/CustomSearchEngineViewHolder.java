/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.browser.search_engines.R;

public class CustomSearchEngineViewHolder extends RecyclerView.ViewHolder {
    private final ImageView mSearchEngineLogo;
    private final TextView mSearchEngineText;
    private final ImageView mDeleteIcon;
    private final View mView;

    CustomSearchEngineViewHolder(View itemView) {
        super(itemView);
        mView = itemView;
        mSearchEngineLogo = itemView.findViewById(R.id.search_engine_logo);
        mSearchEngineText = itemView.findViewById(R.id.search_engine_text);
        mDeleteIcon = itemView.findViewById(R.id.search_engine_delete_icon);
    }

    public ImageView getSearchEngineLogo() {
        return mSearchEngineLogo;
    }

    public TextView getSearchEngineText() {
        return mSearchEngineText;
    }

    public ImageView getDeleteIcon() {
        return mDeleteIcon;
    }

    public View getView() {
        return mView;
    }
}
