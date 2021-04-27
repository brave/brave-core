/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;

import java.util.List;

public class BraveSearchEngineAdapter extends SearchEngineAdapter {
    public static final String PRIVATE_DSE_SHORTNAME = "private_dse_shortname";
    public static final String STANDARD_DSE_SHORTNAME = "standard_dse_shortname";

    private boolean mIsPrivate;

    static public void setDSEPrefs(TemplateUrl templateUrl, boolean isPrivate) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putString(
                isPrivate ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                templateUrl.getShortName());
        sharedPreferencesEditor.apply();
    }

    static public void updateActiveDSE(boolean isPrivate) {
        TemplateUrl templateUrl = getTemplateUrlByShortName(getDSEShortName(isPrivate));
        if (templateUrl == null) {
            return;
        }
        String keyword = templateUrl.getKeyword();
        TemplateUrlServiceFactory.get().setSearchEngine(keyword);
    }

    static public String getDSEShortName(boolean isPrivate) {
        String defaultSearchEngineName = null;
        TemplateUrl dseTemplateUrl =
                TemplateUrlServiceFactory.get().getDefaultSearchEngineTemplateUrl();
        if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();

        return ContextUtils.getAppSharedPreferences().getString(
                isPrivate ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                defaultSearchEngineName);
    }

    static public TemplateUrl getTemplateUrlByShortName(String name) {
        List<TemplateUrl> templateUrls = TemplateUrlServiceFactory.get().getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
            TemplateUrl templateUrl = templateUrls.get(index);
            if (templateUrl.getShortName().equals(name)) {
                return templateUrl;
            }
        }
        assert false : "This should not happen!";
        return null;
    }

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
        setDSEPrefs(templateUrl, mIsPrivate);
    }

    @Override
    public void start() {
        updateActiveDSE(mIsPrivate);
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
