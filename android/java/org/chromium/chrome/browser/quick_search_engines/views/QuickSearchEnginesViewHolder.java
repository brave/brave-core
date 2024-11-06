/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.views;

import android.view.View;
import android.widget.ImageView;

import androidx.recyclerview.widget.RecyclerView;

public class QuickSearchEnginesViewHolder extends RecyclerView.ViewHolder {
    ImageView mSearchEngineLogo;

    QuickSearchEnginesViewHolder(View itemView) {
        super(itemView);
        mSearchEngineLogo = (ImageView) itemView;
    }
}
