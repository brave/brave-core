/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.settings.BraveOriginPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;

/**
 * Routes the Origin promo deep link (https://brave.com/deeplink-android-origin-promo) to the right
 * in-app destination: - Active subscribers: Origin settings page. - Non-subscribers: Origin payment
 * screen (BraveOriginPlansActivity).
 */
@NullMarked
public final class BraveOriginDeepLinkHandler {
    /** Path token used by both the App Link URL and the Play Install Referrer string. */
    public static final String PATH_TOKEN = "deeplink-android-origin-promo";

    private BraveOriginDeepLinkHandler() {}

    /**
     * If {@code intent} is the Origin promo deep link, neutralize its action/data so upstream URL
     * handling skips it and return {@code true}. Caller should then call {@link #open}.
     */
    public static boolean consumeFromIntent(@Nullable Intent intent) {
        if (intent == null || !Intent.ACTION_VIEW.equals(intent.getAction())) {
            return false;
        }
        Uri data = intent.getData();
        if (data == null || !PATH_TOKEN.equalsIgnoreCase(data.getLastPathSegment())) {
            return false;
        }
        intent.setData(null);
        intent.setAction(Intent.ACTION_MAIN);
        return true;
    }

    /**
     * Reads and clears the deferred pref written by the Play Install Referrer flow. Returns {@code
     * true} if the pref was set. Caller should then call {@link #open}.
     */
    public static boolean consumeDeferred() {
        if (!ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, false)) {
            return false;
        }
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, false);
        return true;
    }

    /**
     * Opens the Origin destination — settings for subscribers, payment screen for everyone else.
     * The subscription cache is refreshed in {@code BraveActivity.finishNativeInitialization}, so
     * it reflects the most recent known state.
     */
    public static void open(Activity activity) {
        if (BraveOriginSubscriptionPrefs.getIsCredentialSummaryActiveCached()) {
            SettingsNavigationFactory.createSettingsNavigation()
                    .startSettings(activity, BraveOriginPreferences.class);
        } else {
            activity.startActivity(new Intent(activity, BraveOriginPlansActivity.class));
        }
    }
}
