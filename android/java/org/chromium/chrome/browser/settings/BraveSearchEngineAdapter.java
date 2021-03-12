/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.components.search_engines.TemplateUrl;

public class BraveSearchEngineAdapter extends SearchEngineAdapter {
    private boolean mIsPrivate;

    public BraveSearchEngineAdapter(Context context, boolean isPrivate) {
        super(context);
        mIsPrivate = isPrivate;
    }

    @Override
    public void onClick(View view) {
        super.onClick(view);

        if (view.getTag() == null) {
            return;
        }

        TemplateUrl templateUrl = (TemplateUrl) getItem((int) view.getTag());
        BraveSearchEngineUtils.setDSEPrefs(templateUrl, mIsPrivate);
    }

    @Override
    public void start() {
        BraveSearchEngineUtils.updateActiveDSE(mIsPrivate);
        super.start();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        TextView url = (TextView) view.findViewById(R.id.url);
        if (url != null) {
            url.setVisibility(View.GONE);
        }
        return view;
    }
}
