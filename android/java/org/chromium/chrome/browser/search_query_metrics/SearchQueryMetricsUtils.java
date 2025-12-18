/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_query_metrics;

import android.content.Context;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.content_public.browser.WebContents;

/** Utility class for accessing Search Query Metrics from Java. */
@NullMarked
@JNINamespace("metrics::utils")
public class SearchQueryMetricsUtils {
    /**
     * Marks the entry point as a Direct Navigation.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsDirect(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsDirect(webContents);
    }

    /**
     * Marks the entry point as an Omnibox History.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsOmniboxHistory(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsOmniboxHistory(webContents);
    }

    /**
     * Marks the entry point as an Omnibox Suggestion.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsOmniboxSuggestion(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsOmniboxSuggestion(webContents);
    }

    /**
     * Marks the entry point as an Omnibox Search.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsOmniboxSearch(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsOmniboxSearch(webContents);
    }

    /**
     * Marks the entry point as a Quick Search Engine.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsQuickSearch(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsQuickSearch(webContents);
    }

    /**
     * Marks the entry point as a Shortcut.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsShortcut(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsShortcut(webContents);
    }

    /**
     * Marks the entry point as a Top Site.
     *
     * @param webContents The current active WebContents.
     */
    public static void markEntryPointAsTopSite(WebContents webContents) {
        SearchQueryMetricsUtilsJni.get().markEntryPointAsTopSite(webContents);
    }

    @CalledByNative
    private static boolean isDefaultBrowser() {
        Context context = ContextUtils.getApplicationContext();
        return BraveSetDefaultBrowserUtils.isAppSetAsDefaultBrowser(context);
    }

    @NativeMethods
    public interface Natives {
        void markEntryPointAsDirect(WebContents webContents);

        void markEntryPointAsOmniboxHistory(WebContents webContents);

        void markEntryPointAsOmniboxSuggestion(WebContents webContents);

        void markEntryPointAsOmniboxSearch(WebContents webContents);

        void markEntryPointAsQuickSearch(WebContents webContents);

        void markEntryPointAsShortcut(WebContents webContents);

        void markEntryPointAsTopSite(WebContents webContents);
    }
}
