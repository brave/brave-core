/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.ArrayList;
import java.util.List;

public class CustomSearchEnginesUtil {

    private static final String CUSTOM_SEARCH_ENGINES = "custom_search_engines";

    public static void addCustomSearchEngine(String searchEngineKeyword) {
        List<String> customSearchEnginesList = getCustomSearchEngines();
        if (customSearchEnginesList.size() == 0) {
            customSearchEnginesList = new ArrayList();
        }
        customSearchEnginesList.add(searchEngineKeyword);

        try {
            JSONArray customSearchEnginesJsonArray = new JSONArray();
            for (String customSearchEngineKeyword : customSearchEnginesList) {
                customSearchEnginesJsonArray.put(customSearchEngineKeyword);
            }
            ChromeSharedPreferences.getInstance()
                    .writeString(CUSTOM_SEARCH_ENGINES, customSearchEnginesJsonArray.toString());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static List<String> getCustomSearchEngines() {
        List<String> customSearchEnginesList = new ArrayList();
        String customSearchEngines =
                ChromeSharedPreferences.getInstance().readString(CUSTOM_SEARCH_ENGINES, null);
        if (customSearchEngines == null) {
            return customSearchEnginesList;
        }
        customSearchEngines = "{\"customSearchEngines\":" + customSearchEngines + "}";
        try {
            JSONObject result = new JSONObject(customSearchEngines);
            JSONArray customSearchEnginesJsonArray = result.getJSONArray("customSearchEngines");
            for (int i = 0; i < customSearchEnginesJsonArray.length(); i++) {
                String customSearchEngineKeyword = customSearchEnginesJsonArray.getString(i);
                customSearchEnginesList.add(customSearchEngineKeyword);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return customSearchEnginesList;
    }

    public static boolean isCustomSearchEngineAdded(String searchEngineKeyword) {
        List<String> customSearchEnginesList = getCustomSearchEngines();
        if (customSearchEnginesList.size() > 0
                && customSearchEnginesList.contains(searchEngineKeyword)) {
            return true;
        }
        return false;
    }

    public static void removeCustomSearchEngine(String searchEngineKeyword) {
        List<String> customSearchEnginesList = getCustomSearchEngines();
        if (customSearchEnginesList.size() > 0
                && customSearchEnginesList.contains(searchEngineKeyword)) {
            customSearchEnginesList.remove(searchEngineKeyword);
        }
    }
}
