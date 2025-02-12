/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;


import org.json.JSONObject;
import org.json.JSONException;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;


public class SharedPreferencesHelper {
    public <K, V> void saveMap(String key, Map<K, V> map) {
        JSONObject jsonObject = new JSONObject();
        try {
            for (Map.Entry<K, V> entry : map.entrySet()) {
                jsonObject.put(String.valueOf(entry.getKey()), entry.getValue());
            }
            String json = jsonObject.toString();
            if (json != null && !json.isEmpty()) {
                ChromeSharedPreferences.getInstance().writeString(key, json);
            }
        } catch (JSONException e) {
            // Handle or log error
        }
    }

    public <K, V> Map<K, V> getMap(String key, Class<K> keyClass, Class<V> valueClass) {
        String json = ChromeSharedPreferences.getInstance().readString(key, "");
        if (!json.isEmpty()) {
            try {
                JSONObject jsonObject = new JSONObject(json);
                Map<K, V> map = new HashMap<>();
                Iterator<String> keys = jsonObject.keys();
                while (keys.hasNext()) {
                    String mapKey = keys.next();
                    K typedKey = keyClass.cast(convertStringToType(mapKey, keyClass));
                    V typedValue = valueClass.cast(jsonObject.get(mapKey));
                    map.put(typedKey, typedValue);
                }
                return map;
            } catch (JSONException e) {
                return null;
            }
        }
        return null;
    }

    private Object convertStringToType(String value, Class<?> targetClass) {
        if (targetClass == String.class) return value;
        if (targetClass == Integer.class) return Integer.parseInt(value);
        if (targetClass == Long.class) return Long.parseLong(value);
        if (targetClass == Double.class) return Double.parseDouble(value);
        if (targetClass == Boolean.class) return Boolean.parseBoolean(value);
        return value; // fallback to string if type is not handled
    }
}
