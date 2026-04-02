/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.HashSet;
import java.util.Set;

@NullMarked
public class CustomSearchEnginesPrefManager {
    private final SharedPreferencesManager mSharedPreferencesManager;

    private static @Nullable CustomSearchEnginesPrefManager sInstance;

    private CustomSearchEnginesPrefManager() {
        mSharedPreferencesManager = ChromeSharedPreferences.getInstance();
    }

    public static CustomSearchEnginesPrefManager getInstance() {
        if (sInstance == null) {
            sInstance = new CustomSearchEnginesPrefManager();
        }
        return sInstance;
    }

    public void saveCustomSearchEngines(Set<String> customSearchEngines) {
        mSharedPreferencesManager.writeStringSet(
                BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS, customSearchEngines);
    }

    public Set<String> getCustomSearchEngines() {
        // Return a mutable copy so callers can freely modify the set before saving.
        return new HashSet<>(
                mSharedPreferencesManager.readStringSet(
                        BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS));
    }
}
