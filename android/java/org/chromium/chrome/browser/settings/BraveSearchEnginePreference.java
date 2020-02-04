/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.View;
import android.widget.ListView;

import org.chromium.chrome.R;

public class BraveSearchEnginePreference extends ListFragment {
    private SearchEngineAdapter mSearchEngineAdapter;
    private boolean mPrivate;

    public BraveSearchEnginePreference(boolean isPrivate) {
        mPrivate = isPrivate;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(mPrivate ? R.string.prefs_private_search_engine
                                        : R.string.prefs_standard_search_engine);
        mSearchEngineAdapter = new BraveSearchEngineAdapter(getActivity(), mPrivate);
        setListAdapter(mSearchEngineAdapter);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ListView listView = getListView();
        listView.setDivider(null);
        listView.setItemsCanFocus(true);
    }

    @Override
    public void onStart() {
        super.onStart();
        mSearchEngineAdapter.start();
    }

    @Override
    public void onStop() {
        super.onStop();
        mSearchEngineAdapter.stop();
    }
}
