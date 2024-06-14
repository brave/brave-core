/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
 
package org.chromium.chrome.browser.util;

import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;

import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.Map;

public class SharedPreferencesHelper {
    public <K, V> void saveMap(String key, Map<K, V> map) {
        String json = new GsonBuilder().create().toJson(map);
        if (json != null && !json.isEmpty()) {
            ChromeSharedPreferences.getInstance().writeString(key, json);
        }
    }

    public <K, V> Map<K, V> getMap(String key, Class keyClass, Class valueClass) {
        String json = ChromeSharedPreferences.getInstance().readString(key, "");
        if (!json.isEmpty()) {
            return new GsonBuilder()
                    .create()
                    .fromJson(
                            json,
                            TypeToken.getParameterized(Map.class, keyClass, valueClass).getType());
        }
        return null;
    }
}
