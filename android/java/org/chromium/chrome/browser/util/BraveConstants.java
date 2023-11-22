/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

public final class BraveConstants {
    // used in BraveRewardsPanel
    public static final int SITE_BANNER_REQUEST_CODE = 33;
    public static final int VERIFY_WALLET_ACTIVITY_REQUEST_CODE = 34;
    public static final int USER_WALLET_ACTIVITY_REQUEST_CODE = 35;
    public static final int MONTHLY_CONTRIBUTION_REQUEST_CODE = 36;

    // used in BravseSetDefaultBrowserUtils
    public static final int DEFAULT_BROWSER_ROLE_REQUEST_CODE = 37;

    public static final String BRAVE_PRODUCTION_PACKAGE_NAME = "com.brave.browser";
    public static final String BRAVE_BETA_PACKAGE_NAME = "com.brave.browser_beta";
    public static final String BRAVE_NIGHTLY_PACKAGE_NAME = "com.brave.browser_nightly";

    // used in BraveNtpAdapter
    public static final String BRAVE_REF_URL = "https://brave.com/r/";
    public static final String BRAVE_NEWS_LEARN_MORE_URL =
            "https://brave.com/privacy/browser/#brave-news";

    public static final String INDIA_COUNTRY_CODE = "IN";

    public static final String BRAVE_NEWS_PREFERENCES_TYPE = "BraveNewsPreferencesType";

    // Deeplinks
    public static final String DEEPLINK_ANDROID_PLAYLIST = "deeplink-android-playlist";
    public static final String DEEPLINK_ANDROID_VPN = "deeplink-android-vpn";

    public static final String YOUTUBE_DOMAIN = "youtube.com";
}
