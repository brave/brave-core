/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import androidx.appcompat.widget.SearchView;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;

public class SearchPreference extends Preference {
    public SearchPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.custom_layout_preference);
        setWidgetLayoutResource(R.layout.search_preference);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        SearchView searchView = (SearchView) holder.findViewById(R.id.search_preference);

        searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String s) {
                getOnPreferenceChangeListener().onPreferenceChange(SearchPreference.this, s);
                return false;
            }

            @Override
            public boolean onQueryTextChange(String s) {
                getOnPreferenceChangeListener().onPreferenceChange(SearchPreference.this, s);
                return false;
            }
        });
    }
}
