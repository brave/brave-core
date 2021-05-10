/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.os.Bundle;
import android.view.View;
import android.widget.ListView;

import androidx.fragment.app.ListFragment;

import org.chromium.chrome.browser.search_engines.R;

public class BraveSearchEnginePreference extends SearchEngineSettings {
    // These members will be deleted in bytecode, member from parent class will be used instead.
    private SearchEngineAdapter mSearchEngineAdapter;

    // Own members.
    private boolean mPrivate;

    public BraveSearchEnginePreference(boolean isPrivate) {
        mPrivate = isPrivate;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(mPrivate ? R.string.prefs_private_search_engine
                                        : R.string.prefs_standard_search_engine);
    }

    public void createAdapterIfNecessary() {
        if (mSearchEngineAdapter != null) return;
        mSearchEngineAdapter = new BraveSearchEngineAdapter(getActivity(), mPrivate);
    }
}
