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
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesModel;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.util.SharedPreferencesHelper;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchEnginesUtil {
    private static final String YOUTUBE_SEARCH_ENGINE_URL =
            "https://www.youtube.com/results?search_query={searchTerms}";
    public static final String GOOGLE_SEARCH_ENGINE_URL =
            "https://www.google.com/search?q={searchTerms}";

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

    public static void setPreviousDSE(String dseKeyword) {
        ChromeSharedPreferences.getInstance()
                .writeString(
                        BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE, dseKeyword);
    }

    public static String getPreviousDSE() {
        return ChromeSharedPreferences.getInstance()
                .readString(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE, "");
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
        if (getPreviousDSE().isEmpty()) {
            setPreviousDSE(defaultSearchEngineTemplateUrl.getShortName());
        }

        if (!defaultSearchEngineTemplateUrl.getShortName().equals(getPreviousDSE())) {
            addPreviousDSE(profile, searchEnginesMap, defaultSearchEngineTemplateUrl);
        } else {
            for (TemplateUrl templateUrl : templateUrls) {
                if (!searchEnginesMap.containsKey(templateUrl.getKeyword())) {
                    addSearchEngines(searchEnginesMap, templateUrl);
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
            removeDefaultSearchEngine(searchEnginesMap, defaultSearchEngineTemplateUrl);
        }
        saveSearchEnginesIntoPref(searchEnginesMap);
        return searchEnginesMap;
    }

    private static void addPreviousDSE(
            Profile profile,
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl defaultSearchEngineTemplateUrl) {
        // Create new map to store search engines with previous DSE first
        Map<String, QuickSearchEnginesModel> orderedSearchEnginesMap =
                new LinkedHashMap<String, QuickSearchEnginesModel>();

        // Get previous default search engine template URL
        TemplateUrl previousDSETemplateUrl =
                BraveSearchEngineUtils.getTemplateUrlByShortName(profile, getPreviousDSE());

        // Remove previous DSE from current map if it exists
        searchEnginesMap.remove(previousDSETemplateUrl.getKeyword());

        // Remove current default search engine
        removeDefaultSearchEngine(searchEnginesMap, defaultSearchEngineTemplateUrl);

        // Add previous DSE as first entry in ordered map
        addSearchEngines(orderedSearchEnginesMap, previousDSETemplateUrl);

        // Add remaining search engines after previous DSE
        orderedSearchEnginesMap.putAll(searchEnginesMap);

        // Update reference to ordered map
        searchEnginesMap.clear();
        searchEnginesMap.putAll(orderedSearchEnginesMap);

        // Update previous DSE to current default
        setPreviousDSE(defaultSearchEngineTemplateUrl.getShortName());
    }

    private static void removeDefaultSearchEngine(
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl defaultSearchEngineTemplateUrl) {
        if (searchEnginesMap.containsKey(defaultSearchEngineTemplateUrl.getKeyword())) {
            searchEnginesMap.remove(defaultSearchEngineTemplateUrl.getKeyword());
        }
    }

    private static void addSearchEngines(
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl searchEngineTemplateUrl) {
        QuickSearchEnginesModel quickSearchEnginesModel =
                new QuickSearchEnginesModel(
                        searchEngineTemplateUrl.getShortName(),
                        searchEngineTemplateUrl.getKeyword(),
                        searchEngineTemplateUrl.getURL(),
                        true);
        searchEnginesMap.put(searchEngineTemplateUrl.getKeyword(), quickSearchEnginesModel);
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
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        QuickSearchEnginesModel defaultSearchEnginesModel =
                new QuickSearchEnginesModel(
                        defaultSearchEngineTemplateUrl.getShortName(),
                        defaultSearchEngineTemplateUrl.getKeyword(),
                        defaultSearchEngineTemplateUrl.getURL(),
                        true);
        return defaultSearchEnginesModel;
    }
}
