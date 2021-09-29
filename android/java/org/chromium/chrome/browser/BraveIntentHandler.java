/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;

import org.chromium.base.IntentUtils;

public class BraveIntentHandler extends IntentHandler {
    private static final String CONNECTION_INFO_HELP_URL =
            "https://support.google.com/chrome?p=android_connection_info";
    private static final String BRAVE_CONNECTION_INFO_HELP_URL =
            "https://support.brave.com/hc/en-us/articles/360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";

    public BraveIntentHandler(Activity activity, IntentHandlerDelegate delegate) {
        super(activity, delegate);
    }

    @Override
    public boolean onNewIntent(Intent intent) {
        // Redirect requests if necessary
        String url = getUrlFromIntent(intent);
        if (url != null && url.equals(CONNECTION_INFO_HELP_URL)) {
            intent.setData(Uri.parse(BRAVE_CONNECTION_INFO_HELP_URL));
        }
        return super.onNewIntent(intent);
    }

    /**
     * Helper method to extract the raw URL from the intent, without further processing.
     * The URL may be in multiple locations.
     * @param intent Intent to examine.
     * @return Raw URL from the intent, or null if raw URL could't be found.
     */
    protected static String extractUrlFromIntent(Intent intent) {
        if (intent == null) return null;
        String url = getUrlFromVoiceSearchResult(intent);
        if (url == null) url = getUrlForCustomTab(intent);
        if (url == null) url = getUrlForWebapp(intent);
        if (url == null) url = intent.getDataString();
        if (url == null) url = getUrlFromText(intent);
        if (url == null) return null;
        url = url.trim();
        return TextUtils.isEmpty(url) ? null : url;
    }

    protected static String getUrlFromText(Intent intent) {
        if (intent == null) return null;
        String text = IntentUtils.safeGetStringExtra(intent, Intent.EXTRA_TEXT);
        return (text == null || isJavascriptSchemeOrInvalidUrl(text)) ? null : text;
    }

    private static String getUrlForCustomTab(Intent intent) {
        assert (false);
        return null;
    }

    private static String getUrlForWebapp(Intent intent) {
        assert (false);
        return null;
    }

    private static boolean isJavascriptSchemeOrInvalidUrl(String url) {
        assert (false);
        return false;
    }
}
