/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.SwitchCompat;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

public class QuickSearchViewHolder extends RecyclerView.ViewHolder {
    ImageView searchEngineLogo;
    TextView searchEngineText;
    SwitchCompat toggleSwitch;

    QuickSearchViewHolder(View itemView) {
        super(itemView);
        this.searchEngineLogo = (ImageView) itemView.findViewById(R.id.search_engine_logo);
        this.searchEngineText = (TextView) itemView.findViewById(R.id.search_engine_text);
        this.toggleSwitch = (SwitchCompat) itemView.findViewById(R.id.search_engine_switch);
    }
}
