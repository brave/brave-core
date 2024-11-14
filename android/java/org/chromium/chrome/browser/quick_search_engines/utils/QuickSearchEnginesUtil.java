/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.utils;

import android.content.Context;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.util.SharedPreferencesHelper;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesModel;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchEnginesUtil {
    private static final String YOUTUBE_SEARCH_ENGINE_URL =
            "https://www.youtube.com/results?search_query={searchTerms}";

    public static void saveSearchEnginesIntoPref(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        new SharedPreferencesHelper()
                .saveMap(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES, searchEnginesMap);
    }

    public static Map<String, QuickSearchEnginesModel> getQuickSearchEnginesFromPref() {
        SharedPreferencesHelper sharedPreferencesHelper = new SharedPreferencesHelper();
        Map<String, QuickSearchEnginesModel> searchEnginesMap =
                sharedPreferencesHelper.getMap(
                        BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES,
                        String.class,
                        QuickSearchEnginesModel.class);
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

    private static Map<String, QuickSearchEnginesModel> getQuickSearchEngines(Profile profile) {
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                templateUrls,
                defaultSearchEngineTemplateUrl,
                templateUrlService.isEeaChoiceCountry());

        Map<String, QuickSearchEnginesModel> searchEnginesMap =
                getQuickSearchEnginesFromPref() != null
                        ? getQuickSearchEnginesFromPref()
                        : new LinkedHashMap<String, QuickSearchEnginesModel>();
        for (TemplateUrl templateUrl : templateUrls) {
            if (!searchEnginesMap.containsKey(templateUrl.getKeyword())) {
                QuickSearchEnginesModel quickSearchEnginesModel =
                        new QuickSearchEnginesModel(
                                templateUrl.getShortName(),
                                templateUrl.getKeyword(),
                                templateUrl.getURL(),
                                true);
                searchEnginesMap.put(templateUrl.getKeyword(), quickSearchEnginesModel);
                if (BraveActivity.GOOGLE_SEARCH_ENGINE_KEYWORD.equals(templateUrl.getKeyword())
                        && !searchEnginesMap.containsKey(
                                BraveActivity.YOUTUBE_SEARCH_ENGINE_KEYWORD)) {
                    addYtQuickSearchEnginesModel(searchEnginesMap);
                }
            }
        }
        if (!searchEnginesMap.containsKey(BraveActivity.GOOGLE_SEARCH_ENGINE_KEYWORD)) {
            addYtQuickSearchEnginesModel(searchEnginesMap);
        }
        saveSearchEnginesIntoPref(searchEnginesMap);
        return searchEnginesMap;
    }

    private static void addYtQuickSearchEnginesModel(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        Context context = ContextUtils.getApplicationContext();
        QuickSearchEnginesModel ytQuickSearchEnginesModel =
                new QuickSearchEnginesModel(
                        context.getResources().getString(R.string.youtube),
                        BraveActivity.YOUTUBE_SEARCH_ENGINE_KEYWORD,
                        YOUTUBE_SEARCH_ENGINE_URL,
                        true);
        searchEnginesMap.put(ytQuickSearchEnginesModel.getKeyword(), ytQuickSearchEnginesModel);
    }

    public static List<QuickSearchEnginesModel> getQuickSearchEnginesForSettings(Profile profile) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getQuickSearchEngines(profile);
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        List<QuickSearchEnginesModel> quickSearchEngines = new ArrayList<>();
        for (Map.Entry<String, QuickSearchEnginesModel> entry : searchEnginesMap.entrySet()) {
            QuickSearchEnginesModel quickSearchEnginesModel = entry.getValue();
            if (!quickSearchEnginesModel
                    .getKeyword()
                    .equals(defaultSearchEngineTemplateUrl.getKeyword())) {
                quickSearchEngines.add(quickSearchEnginesModel);
            }
        }
        return quickSearchEngines;
    }

    public static List<QuickSearchEnginesModel> getQuickSearchEnginesForView(Profile profile) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getQuickSearchEngines(profile);
        List<QuickSearchEnginesModel> quickSearchEngines = new ArrayList<>();
        for (Map.Entry<String, QuickSearchEnginesModel> entry : searchEnginesMap.entrySet()) {
            QuickSearchEnginesModel quickSearchEnginesModel = entry.getValue();
            if (quickSearchEnginesModel.isEnabled()) {
                quickSearchEngines.add(quickSearchEnginesModel);
            }
        }
        return quickSearchEngines;
    }

    public static QuickSearchEnginesModel getDefaultSearchEngine(Profile profile) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getQuickSearchEngines(profile);
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        QuickSearchEnginesModel defaultSearchEnginesModel =
                searchEnginesMap.get(defaultSearchEngineTemplateUrl.getKeyword());
        return defaultSearchEnginesModel;
    }
}
