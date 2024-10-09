/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import org.chromium.components.search_engines.TemplateUrl;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

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

    public static void sortAndFilterUnnecessaryTemplateUrl(
            List<TemplateUrl> templateUrls,
            TemplateUrl defaultSearchEngine,
            boolean isEeaChoiceCountry) {
        int recentEngineNum = 0;
        long displayTime =
                System.currentTimeMillis() - SearchEngineAdapter.MAX_DISPLAY_TIME_SPAN_MS;
        Set<String> templateUrlSet = new HashSet<String>();
        Iterator<TemplateUrl> iterator = templateUrls.iterator();
        while (iterator.hasNext()) {
            TemplateUrl templateUrl = iterator.next();
            if (!templateUrlSet.contains(templateUrl.getShortName())) {
                templateUrlSet.add(templateUrl.getShortName());
            } else {
                iterator.remove();
                continue;
            }
            if (getSearchEngineSourceType(templateUrl, defaultSearchEngine)
                    != SearchEngineAdapter.TemplateUrlSourceType.RECENT) {
                continue;
            }
            if (recentEngineNum < SearchEngineAdapter.MAX_RECENT_ENGINE_NUM
                    && templateUrl.getLastVisitedTime() > displayTime) {
                recentEngineNum++;
            } else {
                iterator.remove();
            }
        }
    }

    public static @SearchEngineAdapter.TemplateUrlSourceType int getSearchEngineSourceType(
            TemplateUrl templateUrl, TemplateUrl defaultSearchEngine) {
        if (templateUrl.getIsPrepopulated()) {
            return SearchEngineAdapter.TemplateUrlSourceType.PREPOPULATED;
        } else if (templateUrl.equals(defaultSearchEngine)) {
            return SearchEngineAdapter.TemplateUrlSourceType.DEFAULT;
        } else {
            return SearchEngineAdapter.TemplateUrlSourceType.RECENT;
        }
    }
}
