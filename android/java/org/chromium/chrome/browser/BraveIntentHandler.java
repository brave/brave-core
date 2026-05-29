/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.SearchManager;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.searchwidget.SearchWidgetProvider;
import org.chromium.components.external_intents.ExternalNavigationHandler;
import org.chromium.content_public.browser.BrowserStartupController;

import java.util.Locale;
import java.util.concurrent.Callable;

@NullMarked
public class BraveIntentHandler {
    private static final String TAG = "BraveIntentHandler";

    private static final String BRAVE_SCHEME = "brave";

    /** An extra to indicate that the intent was triggered from an app widget Leo button. */
    public static final String EXTRA_INVOKED_FROM_APP_WIDGET_LEO =
            "com.android.brave.invoked_from_app_widget_leo";

    /** An extra to indicate that the Leo voice prompt was executed from the app widget. */
    public static final String EXTRA_LEO_VOICE_PROMPT_INVOKED =
            "com.android.brave.leo_voice_prompt_invoked";

    public static final String CONNECTION_INFO_HELP_URL =
            "https://support.google.com/chrome?p=android_connection_info";
    public static final String BRAVE_CONNECTION_INFO_HELP_URL =
            "https://support.brave.app/hc/en-us/articles/360018185871-How-do-I-check-if-a-site-s-connection-is-secure-";
    public static final String FALLBACK_SUPPORT_URL =
            "https://support.google.com/chrome/topic/6069782";
    public static final String BRAVE_FALLBACK_SUPPORT_URL = "https://support.brave.app/hc/en-us";

    private static final String BRAVE_SEARCH_URL = "search.brave.com";
    private static final String SOURCE = "source";
    private static final String ANDROID = "android";
    private static final String ANDROID_WIDGET = "android-widget";

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
        if (TextUtils.isEmpty(url)) return null;
        return maybeReplaceWidgetSearchSource(intent, url);
    }

    private static String maybeReplaceWidgetSearchSource(final Intent intent, final String url) {
        if (!IntentUtils.safeGetBooleanExtra(
                intent, SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, false)) {
            return url;
        }

        final Uri parsedUrl = Uri.parse(url);
        final String host = parsedUrl.getHost();
        final String source = parsedUrl.getQueryParameter(SOURCE);
        if (!BRAVE_SEARCH_URL.equals(host) || !ANDROID.equals(source)) {
            return url;
        }

        // Rewrite only the `source` parameter on the raw encoded query, so
        // every other parameter's bytes pass through completely untouched.
        //
        // We can't safely decode and re-encode the query through
        // Uri.getQueryParameters() / appendQueryParameter(), because the two
        // sides use different encodings:
        //
        //   - Search URLs use application/x-www-form-urlencoded, where ' '
        //     is encoded as '+' and a literal '+' as '%2B'.
        //   - Android's Uri follows RFC 3986, where ' ' is '%20' and '+' is
        //     a literal.
        //
        // getQueryParameters() percent-decodes '%2B' to '+' but leaves a
        // form-encoded '+' (meaning space) alone. After that step, a
        // literal '+' and an encoded space are indistinguishable, so any
        // attempt to re-encode the value corrupts the query.
        //
        // For example, a widget search for "C++ tutorial" arrives as
        // "q=C%2B%2B+tutorial". getQueryParameters() would return "C+++tutorial"
        // (three '+' chars, origin unknown). If those were then re-encoded
        // and the '+' chars converted back to spaces, the server would
        // receive "C   tutorial"; if they were left as '+', the server
        // would receive a literal "C+++tutorial". Either way the query is
        // broken.
        final String encodedQuery = parsedUrl.getEncodedQuery();
        if (encodedQuery == null) {
            return url;
        }

        final StringBuilder newQuery = new StringBuilder(encodedQuery.length());
        // Walk each name=value pair in encoded form. We intentionally do not
        // use Uri.getQueryParameters() here. See the comment above for why
        // decoding and re-encoding through it is lossy.
        for (String pair : encodedQuery.split("&")) {
            // Restore the '&' separator between pairs (skipped before the
            // first pair).
            //noinspection SizeReplaceableByIsEmpty
            if (newQuery.length() > 0) {
                newQuery.append('&');
            }
            // Locate the name/value split.
            final int eq = pair.indexOf('=');
            // Decode only for the comparison, the original encoded bytes are
            // what we actually write back out below.
            if (eq >= 0
                    && SOURCE.equals(Uri.decode(pair.substring(0, eq)))
                    && ANDROID.equals(Uri.decode(pair.substring(eq + 1)))) {
                // Keep the original "source=" prefix (preserving any
                // encoding the caller used in the name) and substitute just
                // the value. Uri.encode() percent-encodes ANDROID_WIDGET
                // safely, though "android-widget" has no chars that need it.
                newQuery.append(pair, 0, eq + 1).append(Uri.encode(ANDROID_WIDGET));
            } else {
                // Any other parameter including the search query `q`
                // passes through untouched, byte-for-byte.
                newQuery.append(pair);
            }
        }
        // encodedQuery() tells Uri.Builder our string is
        // already encoded and must not be re-encoded.
        return parsedUrl.buildUpon().encodedQuery(newQuery.toString()).build().toString();
    }

    @Nullable
    protected static String getUrlFromText(Intent intent) {
        if (intent == null) return null;
        String text = IntentUtils.safeGetStringExtra(intent, Intent.EXTRA_TEXT);
        if (text == null) return null;
        String urlScheme = ExternalNavigationHandler.getSanitizedUrlScheme(text);
        if (ExternalIntentUrlChecker.isUnsafeExternalScheme(urlScheme)) return null;
        return text;
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

    /**
     * Bytecode-redirected from {@link IntentHandler#isUrlUnsafe}. Defers to the upstream check
     * (which handles chrome://, chrome-native://, devtools://, distiller://, about://) and
     * additionally blocks brave://, since it is a display alias for chrome:// and gets rewritten to
     * chrome:// deeper in the navigation stack — too late to protect this guard.
     */
    public static boolean isUrlUnsafe(@Nullable String url) {
        if (BraveIntentHandlerInternal.isUrlUnsafe(url)) {
            return true;
        }
        if (url == null) return false;
        String scheme = Uri.parse(url).getScheme();
        return scheme != null && BRAVE_SCHEME.equals(scheme.toLowerCase(Locale.US));
    }
}
