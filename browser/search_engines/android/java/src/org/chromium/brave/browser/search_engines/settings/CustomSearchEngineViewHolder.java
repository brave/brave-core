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
    ImageView mSearchEngineLogo;
    TextView mSearchEngineText;
    ImageView mDeleteIcon;
    View mView;

    CustomSearchEngineViewHolder(View itemView) {
        super(itemView);
        mView = itemView;
        mSearchEngineLogo = (ImageView) itemView.findViewById(R.id.search_engine_logo);
        mSearchEngineText = (TextView) itemView.findViewById(R.id.search_engine_text);
        mDeleteIcon = (ImageView) itemView.findViewById(R.id.search_engine_delete_icon);
    }
}
