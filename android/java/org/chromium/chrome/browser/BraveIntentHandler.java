/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.SearchManager;
import android.content.Intent;
import android.text.TextUtils;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.content_public.browser.BrowserStartupController;

import java.util.concurrent.Callable;

@NullMarked
public class BraveIntentHandler {
    private static final String TAG = "BraveIntentHandler";

    /** An extra to indicate that the intent was triggered from an app widget Leo button. */
    public static final String EXTRA_INVOKED_FROM_APP_WIDGET_LEO =
            "com.android.brave.invoked_from_app_widget_leo";

    /** An extra to indicate that the Leo voice prompt was executed from the app widget. */
    public static final String EXTRA_LEO_VOICE_PROMPT_INVOKED =
            "com.android.brave.leo_voice_prompt_invoked";

    public static final String CONNECTION_INFO_HELP_URL =
            "https://support.google.com/chrome?p=android_connection_info";
    public static final String BRAVE_CONNECTION_INFO_HELP_URL =
            "https://support.brave.com/hc/en-us/articles/360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";
    public static final String FALLBACK_SUPPORT_URL =
            "https://support.google.com/chrome/topic/6069782";
    public static final String BRAVE_FALLBACK_SUPPORT_URL = "https://support.brave.com/hc/en-us";

    /**
     * Helper method to extract the raw URL from the intent, without further processing. The URL may
     * be in multiple locations.
     *
     * @param intent Intent to examine.
     * @return Raw URL from the intent, or null if raw URL could't be found.
     */
    @Nullable
    protected static String extractUrlFromIntent(@Nullable Intent intent) {
        if (intent == null) return null;
        String url = IntentHandler.getUrlFromVoiceSearchResult(intent);
        if (url == null) url = getUrlForCustomTab(intent);
        if (url == null) url = getUrlForWebapp(intent);
        if (url == null) url = IntentHandler.getUrlFromShareIntent(intent);
        if (url == null) url = intent.getDataString();
        if (url == null) url = getUrlFromText(intent);
        if (url == null) url = getWebSearchUrl(intent);
        if (url == null) return null;
        url = url.trim();
        return TextUtils.isEmpty(url) ? null : url;
    }

    @Nullable
    protected static String getUrlFromText(Intent intent) {
        if (intent == null) return null;
        String text = IntentUtils.safeGetStringExtra(intent, Intent.EXTRA_TEXT);
        return (text == null || isJavascriptSchemeOrInvalidUrl(text)) ? null : text;
    }

    @Nullable
    protected static String getWebSearchUrl(Intent intent) {
        final String action = intent.getAction();
        if (!Intent.ACTION_WEB_SEARCH.equals(action)) {
            return null;
        }

        String query = IntentUtils.safeGetStringExtra(intent, SearchManager.QUERY);
        if (TextUtils.isEmpty(query)
                || !BrowserStartupController.getInstance().isFullBrowserStarted()) {
            return null;
        }

        try {
            return ThreadUtils.runOnUiThreadBlocking(
                    new Callable<String>() {
                        @Override
                        public String call() {
                            return TemplateUrlServiceFactory.getForProfile(
                                            ProfileManager.getLastUsedRegularProfile())
                                    .getUrlForSearchQuery(query);
                        }
                    });
        } catch (Exception e) {
            Log.e(TAG, "Could not retrieve search query: " + e);
        }
        return null;
    }

    @Nullable
    private static String getUrlForCustomTab(Intent unused_intent) {
        assert false;
        return null;
    }

    @Nullable
    private static String getUrlForWebapp(Intent unused_intent) {
        assert false;
        return null;
    }

    private static boolean isJavascriptSchemeOrInvalidUrl(String unused_url) {
        assert false;
        return false;
    }
}
