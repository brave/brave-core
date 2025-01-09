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

/**
 * Utility class for managing quick search engines functionality. Provides methods for saving and
 * retrieving search engine preferences, enabling/disabling the quick search engines feature, and
 * managing default search engine settings.
 */
public class QuickSearchEnginesUtil {
    // Search engine URL templates
    private static final String YOUTUBE_SEARCH_ENGINE_URL =
            "https://www.youtube.com/results?search_query={searchTerms}";
    public static final String GOOGLE_SEARCH_ENGINE_URL =
            "https://www.google.com/search?q={searchTerms}";

    /**
     * Saves the provided search engines map to SharedPreferences
     *
     * @param searchEnginesMap Map of search engine keywords to their QuickSearchEnginesModel
     */
    public static void saveSearchEnginesIntoPref(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        new SharedPreferencesHelper()
                .saveMap(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES, searchEnginesMap);
    }

    /**
     * Retrieves the saved search engines map from SharedPreferences
     *
     * @return Map of search engine keywords to their QuickSearchEnginesModel, or null if not found
     */
    public static Map<String, QuickSearchEnginesModel> getQuickSearchEnginesFromPref() {
        SharedPreferencesHelper sharedPreferencesHelper = new SharedPreferencesHelper();
        return sharedPreferencesHelper.getMap(
                BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES,
                String.class,
                QuickSearchEnginesModel.class);
    }

    /**
     * Sets whether the quick search engines feature should be shown
     *
     * @param shouldShow True to show the feature, false to hide it
     */
    public static void setQuickSearchEnginesFeature(boolean shouldShow) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_FEATURE, shouldShow);
    }

    /**
     * Gets whether the quick search engines feature is enabled
     *
     * @return True if the feature is enabled, false otherwise. Defaults to true.
     */
    public static boolean getQuickSearchEnginesFeature() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_FEATURE, true);
    }

    /**
     * Saves the keyword of the previous default search engine
     *
     * @param dseKeyword Keyword identifying the previous default search engine
     */
    public static void setPreviousDSE(String dseKeyword) {
        ChromeSharedPreferences.getInstance()
                .writeString(
                        BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE, dseKeyword);
    }

    /**
     * Gets the keyword of the previously saved default search engine
     *
     * @return Keyword of the previous default search engine, or empty string if not found
     */
    public static String getPreviousDSE() {
        return ChromeSharedPreferences.getInstance()
                .readString(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE, "");
    }

    /**
     * Gets the list of quick search engines for a given profile
     *
     * @param profile The user profile to get search engines for
     * @return Map of search engine keywords to their QuickSearchEnginesModel
     */
    private static Map<String, QuickSearchEnginesModel> getQuickSearchEngines(Profile profile) {
        // Get template URL service and default search engine
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        TemplateUrl defaultSearchEngine = templateUrlService.getDefaultSearchEngineTemplateUrl();

        // Get sorted and filtered list of template URLs
        List<TemplateUrl> templateUrls =
                getSortedTemplateUrls(templateUrlService, defaultSearchEngine);

        // Get existing or create new search engines map
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getOrCreateSearchEnginesMap();

        // Initialize previous default search engine if not already set
        initializePreviousDSEIfNeeded(defaultSearchEngine);

        // Handle search engine updates based on whether default has changed
        if (hasDefaultSearchEngineChanged(defaultSearchEngine)) {
            addPreviousDSE(profile, searchEnginesMap, defaultSearchEngine);
        } else {
            updateSearchEngines(searchEnginesMap, templateUrls, defaultSearchEngine);
        }

        // Handle YouTube search engine and save preferences
        handleYtSearchEngine(searchEnginesMap);
        saveSearchEnginesIntoPref(searchEnginesMap);
        return searchEnginesMap;
    }

    /** Gets sorted and filtered list of template URLs */
    private static List<TemplateUrl> getSortedTemplateUrls(
            TemplateUrlService service, TemplateUrl defaultSearchEngine) {
        List<TemplateUrl> templateUrls = service.getTemplateUrls();
        // Sort and filter unnecessary template URLs based on region
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                templateUrls, defaultSearchEngine, service.isEeaChoiceCountry());
        return templateUrls;
    }

    /** Gets existing search engines map from preferences or creates new one if none exists */
    private static Map<String, QuickSearchEnginesModel> getOrCreateSearchEnginesMap() {
        Map<String, QuickSearchEnginesModel> existing = getQuickSearchEnginesFromPref();
        return existing != null ? existing : new LinkedHashMap<String, QuickSearchEnginesModel>();
    }

    /** Sets the previous default search engine if not already initialized */
    private static void initializePreviousDSEIfNeeded(TemplateUrl defaultSearchEngine) {
        if (getPreviousDSE().isEmpty()) {
            setPreviousDSE(defaultSearchEngine.getShortName());
        }
    }

    /** Checks if the default search engine has changed from the previous one */
    private static boolean hasDefaultSearchEngineChanged(TemplateUrl defaultSearchEngine) {
        return !defaultSearchEngine.getShortName().equals(getPreviousDSE());
    }

    /** Updates search engines map with new engines and removes default search engine */
    private static void updateSearchEngines(
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            List<TemplateUrl> templateUrls,
            TemplateUrl defaultSearchEngine) {
        // Add any new search engines not already in preferences
        for (TemplateUrl templateUrl : templateUrls) {
            if (!searchEnginesMap.containsKey(templateUrl.getKeyword())) {
                addSearchEngines(searchEnginesMap, templateUrl);
            }
        }
        // Remove default search engine from quick search options
        removeDefaultSearchEngine(searchEnginesMap, defaultSearchEngine);
    }

    /**
     * Handles adding YouTube as a quick search engine option when Google is present
     *
     * @param searchEnginesMap Map of search engine keywords to their QuickSearchEnginesModel
     */
    private static void handleYtSearchEngine(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        // If Google search is not present, just add YouTube search
        if (!searchEnginesMap.containsKey(BraveActivity.GOOGLE_SEARCH_ENGINE_KEYWORD)) {
            addYtQuickSearchEnginesModel(searchEnginesMap);
        } else {
            // Create new map to preserve ordering while adding YouTube after Google
            Map<String, QuickSearchEnginesModel> ytSearchEnginesMap =
                    new LinkedHashMap<String, QuickSearchEnginesModel>();

            // Iterate through existing engines
            for (Map.Entry<String, QuickSearchEnginesModel> entry : searchEnginesMap.entrySet()) {
                QuickSearchEnginesModel quickSearchEnginesModel = entry.getValue();
                ytSearchEnginesMap.put(
                        quickSearchEnginesModel.getKeyword(), quickSearchEnginesModel);

                // Add YouTube search right after Google if it's not already present
                if (BraveActivity.GOOGLE_SEARCH_ENGINE_KEYWORD.equals(
                                quickSearchEnginesModel.getKeyword())
                        && !searchEnginesMap.containsKey(
                                BraveActivity.YOUTUBE_SEARCH_ENGINE_KEYWORD)) {
                    addYtQuickSearchEnginesModel(ytSearchEnginesMap);
                }
            }

            // Replace original map with new ordered map containing YouTube
            searchEnginesMap.clear();
            searchEnginesMap.putAll(ytSearchEnginesMap);
        }
    }

    /**
     * Adds the previous default search engine (DSE) to the quick search engines list. This method:
     * 1. Retrieves the previous DSE from preferences 2. Removes both previous and current DSE from
     * the existing map 3. Creates a new ordered map with previous DSE as first entry 4. Updates the
     * stored previous DSE preference
     *
     * @param profile The user profile to get search engine data from
     * @param searchEnginesMap Map of current quick search engines
     * @param defaultSearchEngineTemplateUrl The current default search engine
     */
    private static void addPreviousDSE(
            Profile profile,
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl defaultSearchEngineTemplateUrl) {
        // Get previous default search engine
        TemplateUrl previousDSETemplateUrl =
                BraveSearchEngineUtils.getTemplateUrlByShortName(profile, getPreviousDSE());

        // Create new ordered map and remove existing entries
        Map<String, QuickSearchEnginesModel> orderedMap = new LinkedHashMap<>();
        searchEnginesMap.remove(previousDSETemplateUrl.getKeyword());
        removeDefaultSearchEngine(searchEnginesMap, defaultSearchEngineTemplateUrl);

        // Rebuild map with previous DSE first
        addSearchEngines(orderedMap, previousDSETemplateUrl);
        orderedMap.putAll(searchEnginesMap);

        // Update the search engines map reference
        searchEnginesMap.clear();
        searchEnginesMap.putAll(orderedMap);

        // Store current DSE as previous for next time
        setPreviousDSE(defaultSearchEngineTemplateUrl.getShortName());
    }

    /**
     * Removes the default search engine from the quick search engines map if present.
     *
     * @param searchEnginesMap Map containing quick search engines
     * @param defaultSearchEngineTemplateUrl The default search engine template URL to remove
     */
    private static void removeDefaultSearchEngine(
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl defaultSearchEngineTemplateUrl) {
        String keyword = defaultSearchEngineTemplateUrl.getKeyword();
        searchEnginesMap.remove(keyword); // Map.remove() returns null if key not present
    }

    /**
     * Adds a search engine to the provided map. Creates a new QuickSearchEnginesModel from the
     * template URL and adds it to the map using the keyword as the key.
     *
     * @param searchEnginesMap Map to add the search engine to
     * @param searchEngineTemplateUrl Template URL containing search engine details
     */
    private static void addSearchEngines(
            Map<String, QuickSearchEnginesModel> searchEnginesMap,
            TemplateUrl searchEngineTemplateUrl) {
        String keyword = searchEngineTemplateUrl.getKeyword();
        QuickSearchEnginesModel quickSearchEnginesModel =
                new QuickSearchEnginesModel(
                        searchEngineTemplateUrl.getShortName(),
                        keyword,
                        searchEngineTemplateUrl.getURL(),
                        true);
        searchEnginesMap.put(keyword, quickSearchEnginesModel);
    }

    /**
     * Adds YouTube as a quick search engine option to the provided map. Creates a new
     * QuickSearchEnginesModel for YouTube and adds it using YouTube's keyword as the key.
     *
     * @param searchEnginesMap Map to add the YouTube search engine to
     */
    private static void addYtQuickSearchEnginesModel(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        Context context = ContextUtils.getApplicationContext();
        String youtubeDisplayName = context.getResources().getString(R.string.youtube);

        QuickSearchEnginesModel youtubeSearchEngine =
                new QuickSearchEnginesModel(
                        youtubeDisplayName,
                        BraveActivity.YOUTUBE_SEARCH_ENGINE_KEYWORD,
                        YOUTUBE_SEARCH_ENGINE_URL,
                        true);

        searchEnginesMap.put(youtubeSearchEngine.getKeyword(), youtubeSearchEngine);
    }

    /**
     * Gets the list of quick search engines for the settings page, excluding the default search
     * engine
     *
     * @param profile The user profile to get search engines for
     * @return List of quick search engines models, excluding the default search engine
     */
    public static List<QuickSearchEnginesModel> getQuickSearchEnginesForSettings(Profile profile) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getQuickSearchEngines(profile);
        String defaultEngineKeyword = getDefaultSearchEngineKeyword(profile);

        List<QuickSearchEnginesModel> quickSearchEngines = new ArrayList<>();
        for (QuickSearchEnginesModel engine : searchEnginesMap.values()) {
            if (!engine.getKeyword().equals(defaultEngineKeyword)) {
                quickSearchEngines.add(engine);
            }
        }
        return quickSearchEngines;
    }

    /**
     * Gets the list of enabled quick search engines for display in the UI
     *
     * @param profile The user profile to get search engines for
     * @return List of enabled quick search engines models
     */
    public static List<QuickSearchEnginesModel> getQuickSearchEnginesForView(Profile profile) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = getQuickSearchEngines(profile);

        List<QuickSearchEnginesModel> quickSearchEngines = new ArrayList<>();
        for (QuickSearchEnginesModel engine : searchEnginesMap.values()) {
            if (engine.isEnabled()) {
                quickSearchEngines.add(engine);
            }
        }
        return quickSearchEngines;
    }

    /**
     * Gets the default search engine as a QuickSearchEnginesModel
     *
     * @param profile The user profile to get the default search engine for
     * @return QuickSearchEnginesModel representing the default search engine
     */
    public static QuickSearchEnginesModel getDefaultSearchEngine(Profile profile) {
        TemplateUrl defaultEngine =
                TemplateUrlServiceFactory.getForProfile(profile)
                        .getDefaultSearchEngineTemplateUrl();

        return new QuickSearchEnginesModel(
                defaultEngine.getShortName(),
                defaultEngine.getKeyword(),
                defaultEngine.getURL(),
                true);
    }

    /** Helper method to get the keyword of the default search engine */
    private static String getDefaultSearchEngineKeyword(Profile profile) {
        return TemplateUrlServiceFactory.getForProfile(profile)
                .getDefaultSearchEngineTemplateUrl()
                .getKeyword();
    }
}
