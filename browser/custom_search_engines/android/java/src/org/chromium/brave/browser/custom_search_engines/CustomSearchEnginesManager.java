/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines;

import android.content.Context;
import android.widget.ImageView;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.GoogleFaviconServerCallback;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.net.NetworkTrafficAnnotationTag;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

@NullMarked
public class CustomSearchEnginesManager {
    public static String KEYWORD = "keyword";

    private static @Nullable CustomSearchEnginesManager sInstance;
    private final CustomSearchEnginesPrefManager mCustomSearchEnginesPrefManager;

    private CustomSearchEnginesManager() {
        mCustomSearchEnginesPrefManager = CustomSearchEnginesPrefManager.getInstance();
    }

    public static CustomSearchEnginesManager getInstance() {
        if (sInstance == null) {
            sInstance = new CustomSearchEnginesManager();
        }
        return sInstance;
    }

    public void addCustomSearchEngine(String searchEngineKeyword) {
        List<String> customSearchEnginesList =
                mCustomSearchEnginesPrefManager.getCustomSearchEngines();
        if (customSearchEnginesList.isEmpty()) {
            customSearchEnginesList = new ArrayList<>();
        }
        if (!customSearchEnginesList.contains(searchEngineKeyword)) {
            customSearchEnginesList.add(searchEngineKeyword);
            mCustomSearchEnginesPrefManager.saveCustomSearchEngines(customSearchEnginesList);
        }
    }

    public boolean isCustomSearchEngineAdded(String searchEngineKeyword) {
        List<String> customSearchEnginesList =
                mCustomSearchEnginesPrefManager.getCustomSearchEngines();
        return !customSearchEnginesList.isEmpty()
                && customSearchEnginesList.contains(searchEngineKeyword);
    }

    public void removeCustomSearchEngine(String searchEngineKeyword) {
        List<String> customSearchEnginesList =
                mCustomSearchEnginesPrefManager.getCustomSearchEngines();
        if (!customSearchEnginesList.isEmpty()
                && customSearchEnginesList.contains(searchEngineKeyword)) {
            customSearchEnginesList.remove(searchEngineKeyword);
            mCustomSearchEnginesPrefManager.saveCustomSearchEngines(customSearchEnginesList);
        }
    }

    public void updateCustomSearchEngine(
            String searchEngineKeyword, String title, String newSearchEngineKeyword, String url) {
        List<String> customSearchEnginesList =
                mCustomSearchEnginesPrefManager.getCustomSearchEngines();
        if (!customSearchEnginesList.isEmpty()
                && customSearchEnginesList.contains(searchEngineKeyword)) {
            int index = customSearchEnginesList.indexOf(searchEngineKeyword);
            if (index != -1) {
                customSearchEnginesList.set(index, newSearchEngineKeyword);
            }
            mCustomSearchEnginesPrefManager.saveCustomSearchEngines(customSearchEnginesList);
        }
    }

    public void loadSearchEngineLogo(
            LargeIconBridge largeIconBridge,
            TemplateUrlService templateUrlService,
            ImageView logoView,
            String searchKeyword) {
        Context context = ContextUtils.getApplicationContext();
        GURL faviconUrl =
                new GURL(templateUrlService.getSearchEngineUrlFromTemplateUrl(searchKeyword));
        int uiElementSizeInPx = 24;
        logoView.setImageBitmap(
                FaviconUtils.createGenericFaviconBitmap(context, uiElementSizeInPx, null));
        LargeIconCallback onFaviconAvailable =
                (icon, fallbackColor, isFallbackColorDefault, iconType) -> {
                    if (icon != null) {
                        logoView.setImageBitmap(icon);
                    }
                };
        GoogleFaviconServerCallback googleServerCallback =
                (status) -> {
                    largeIconBridge.touchIconFromGoogleServer(faviconUrl);
                    largeIconBridge.getLargeIconForUrl(
                            faviconUrl,
                            /* minSizePx= */ 1,
                            /* desiredSizePx= */ uiElementSizeInPx,
                            onFaviconAvailable);
                };
        NetworkTrafficAnnotationTag trafficAnnotation =
                NetworkTrafficAnnotationTag.createComplete(
                        "brave_custom_search_engine_favicon",
                        "semantics {"
                                + "  sender: 'Custom Search Engine Settings'"
                                + "  description: 'Fetches favicon for user-added custom search"
                                + " engines from Google favicon service.'"
                                + "  trigger: 'User opens the Custom Search Engines settings"
                                + " list.'"
                                + "  data: 'The URL of the custom search engine added by the"
                                + " user.'"
                                + "  destination: GOOGLE_OWNED_SERVICE"
                                + "}"
                                + "policy {"
                                + "  cookies_allowed: NO"
                                + "  policy_exception_justification: 'Not implemented.'"
                                + "}");
        largeIconBridge.getLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
                faviconUrl,
                /* shouldTrimPageUrlPath= */ true,
                trafficAnnotation,
                googleServerCallback);
    }
}
