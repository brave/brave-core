/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class CustomSearchEnginesPrefManager {
    private static final String TAG = "CustomSearchEngines";

    private static final String CUSTOM_SEARCH_ENGINES = "custom_search_engines";

    private SharedPreferencesManager mSharedPreferencesManager;

    private static CustomSearchEnginesPrefManager sInstance;

    private CustomSearchEnginesPrefManager() {
        mSharedPreferencesManager = ChromeSharedPreferences.getInstance();
    }

    public static CustomSearchEnginesPrefManager getInstance() {
        if (sInstance == null) {
            sInstance = new CustomSearchEnginesPrefManager();
        }
        return sInstance;
    }

    public void saveCustomSearchEngines(List<String> customSearchEnginesList) {
        if (customSearchEnginesList == null) {
            return;
        }

        try {
            JSONArray jsonArray = new JSONArray(customSearchEnginesList);
            mSharedPreferencesManager.writeString(CUSTOM_SEARCH_ENGINES, jsonArray.toString());
        } catch (Exception e) {
            Log.e(TAG, "Error saving search engines", e);
        }
    }

    public List<String> getCustomSearchEngines() {
        List<String> customSearchEnginesList = new LinkedList<>();
        String savedSearchEngines =
                mSharedPreferencesManager.readString(CUSTOM_SEARCH_ENGINES, null);

        if (savedSearchEngines == null) {
            return customSearchEnginesList;
        }

        try {
            JSONObject wrapper =
                    new JSONObject("{\"customSearchEngines\":" + savedSearchEngines + "}");
            JSONArray searchEnginesArray = wrapper.getJSONArray("customSearchEngines");

            for (int i = 0; i < searchEnginesArray.length(); i++) {
                customSearchEnginesList.add(searchEnginesArray.getString(i));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return customSearchEnginesList;
    }
}
