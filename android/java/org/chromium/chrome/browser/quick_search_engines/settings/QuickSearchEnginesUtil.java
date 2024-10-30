/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.util.SharedPreferencesHelper;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchEnginesUtil {
    public static void saveSearchEnginesIntoPref(
            Map<String, QuickSearchEngineModel> searchEnginesMap) {
        new SharedPreferencesHelper()
                .saveMap(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES, searchEnginesMap);
    }

    public static Map<String, QuickSearchEngineModel> getQuickSearchEnginesFromPref() {
        SharedPreferencesHelper sharedPreferencesHelper = new SharedPreferencesHelper();
        Map<String, QuickSearchEngineModel> searchEnginesMap =
                sharedPreferencesHelper.getMap(
                        BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES,
                        String.class,
                        QuickSearchEngineModel.class);
        return searchEnginesMap;
    }

    public static void setQuickSearchEnginesFeature(boolean showShow) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_FEATURE, showShow);
    }

    public static boolean getQuickSearchEnginesFeature() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_FEATURE, true);
    }

    public static List<QuickSearchEngineModel> getQuickSearchEngines(Profile profile) {
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                templateUrls,
                defaultSearchEngineTemplateUrl,
                templateUrlService.isEeaChoiceCountry());

        List<QuickSearchEngineModel> quickSearchEngines = new ArrayList<>();
        Map<String, QuickSearchEngineModel> searchEnginesMap =
                getQuickSearchEnginesFromPref() != null
                        ? getQuickSearchEnginesFromPref()
                        : new LinkedHashMap<String, QuickSearchEngineModel>();
        for (TemplateUrl templateUrl : templateUrls) {
            if (!searchEnginesMap.containsKey(templateUrl.getKeyword())) {
                QuickSearchEngineModel quickSearchEngineModel =
                        new QuickSearchEngineModel(
                                templateUrl.getShortName(),
                                templateUrl.getKeyword(),
                                templateUrl.getURL(),
                                true);
                searchEnginesMap.put(templateUrl.getKeyword(), quickSearchEngineModel);
            }
        }
        saveSearchEnginesIntoPref(searchEnginesMap);
        for (Map.Entry<String, QuickSearchEngineModel> entry : searchEnginesMap.entrySet()) {
            QuickSearchEngineModel quickSearchEngineModel = entry.getValue();
            if (quickSearchEngineModel.isEnabled()) {
                quickSearchEngines.add(quickSearchEngineModel);
            }
        }
        return quickSearchEngines;
    }
}
