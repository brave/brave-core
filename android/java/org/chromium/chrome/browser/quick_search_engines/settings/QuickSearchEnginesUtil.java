/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.SharedPreferencesHelper;

import java.util.Map;

public class QuickSearchEnginesUtil {
    public static void saveSearchEngines(Map<String, QuickSearchEngineModel> searchEnginesMap) {
        new SharedPreferencesHelper()
                .saveMap(BravePreferenceKeys.BRAVE_QUICK_SEARCH_ENGINES, searchEnginesMap);
    }

    public static Map<String, QuickSearchEngineModel> getSearchEngines() {
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
}
