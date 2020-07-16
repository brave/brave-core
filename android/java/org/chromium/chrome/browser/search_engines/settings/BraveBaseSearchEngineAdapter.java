/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import androidx.annotation.StringRes;

public class BraveBaseSearchEngineAdapter extends BaseAdapter {
    public BraveBaseSearchEngineAdapter() {
    }

    // BaseAdapter:
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
    	return convertView;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public Object getItem(int pos) {
        return null;
    }

    @Override
    public int getCount() {
        return 0;
    }

    @StringRes
    protected int getPermissionsLinkMessage(String url) {
    	return 0;
    }
}
