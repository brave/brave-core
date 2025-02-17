/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.content.Context;
import android.widget.ImageView;

import org.json.JSONArray;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.GoogleFaviconServerCallback;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.net.NetworkTrafficAnnotationTag;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

public class CustomSearchEnginesUtil {

    private static final String CUSTOM_SEARCH_ENGINES = "custom_search_engines";

    private static void saveCustomSearchEngines(List<String> customSearchEnginesList) {
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

    public static void addCustomSearchEngine(String searchEngineKeyword) {
        List<String> customSearchEnginesList = getCustomSearchEngines();
        if (customSearchEnginesList.size() == 0) {
            customSearchEnginesList = new ArrayList();
        }
        customSearchEnginesList.add(searchEngineKeyword);
        saveCustomSearchEngines(customSearchEnginesList);
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
            Log.e(
                    "brave_search",
                    "CustomSearchEnginesUtil.removeCustomSearchEngine : searchEngineKeyword : "
                            + searchEngineKeyword);
            customSearchEnginesList.remove(searchEngineKeyword);
        }
        saveCustomSearchEngines(customSearchEnginesList);
    }

    public static void loadSearchEngineLogo(
            Profile profile, ImageView logoView, String searchKeyword) {
        Context context = ContextUtils.getApplicationContext();
        LargeIconBridge largeIconBridge = new LargeIconBridge(profile);
        TemplateUrlService mTemplateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        GURL faviconUrl =
                new GURL(mTemplateUrlService.getSearchEngineUrlFromTemplateUrl(searchKeyword));
        // Use a placeholder image while trying to fetch the logo.
        int uiElementSizeInPx = 24;
        logoView.setImageBitmap(
                FaviconUtils.createGenericFaviconBitmap(context, uiElementSizeInPx, null));
        LargeIconCallback onFaviconAvailable =
                (icon, fallbackColor, isFallbackColorDefault, iconType) -> {
                    if (icon != null) {
                        logoView.setImageBitmap(icon);
                        largeIconBridge.destroy();
                    }
                };
        GoogleFaviconServerCallback googleServerCallback =
                (status) -> {
                    // Update the time the icon was last requested to avoid automatic eviction
                    // from cache.
                    largeIconBridge.touchIconFromGoogleServer(faviconUrl);
                    // The search engine logo will be fetched from google servers, so the actual
                    // size of the image is controlled by LargeIconService configuration.
                    // minSizePx=1 is used to accept logo of any size.
                    largeIconBridge.getLargeIconForUrl(
                            faviconUrl,
                            /* minSizePx= */ 1,
                            /* desiredSizePx= */ uiElementSizeInPx,
                            onFaviconAvailable);
                };
        // If the icon already exists in the cache no network request will be made, but the
        // callback will be triggered nonetheless.
        largeIconBridge.getLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
                faviconUrl,
                /* shouldTrimPageUrlPath= */ true,
                NetworkTrafficAnnotationTag.MISSING_TRAFFIC_ANNOTATION,
                googleServerCallback);
    }
}
