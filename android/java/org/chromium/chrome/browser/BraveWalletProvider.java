/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser;

import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.chrome.browser.externalnav.BraveExternalNavigationHandler;
import org.chromium.components.external_intents.ExternalNavigationParams;

import java.util.Locale;

// Used from org.chromium.chrome.browser.externalnav
public class BraveWalletProvider {
    public static final String ACTION_VALUE = "authorization";

    public static final String REDIRECT_URL_KEY = "redirect_url";

    public static final String UPHOLD_REDIRECT_URL = "rewards://uphold";
    public static final String GEMINI_REDIRECT_URL = "rewards://gemini";
    public static final String BRAVE_SUPPORT_URL = "https://community.brave.com";
    public static final String UPHOLD_ORIGIN_URL = "http://uphold.com";

    public static final String BITFLYER_REDIRECT_URL = "rewards://bitflyer";

    // Wallet types
    public static final String UPHOLD = "uphold";
    public static final String BITFLYER = "bitflyer";
    public static final String GEMINI = "gemini";

    private static int UNKNOWN_ERROR_CODE = -1;

    public void completeWalletProviderVerification(
            ExternalNavigationParams params, BraveExternalNavigationHandler handler) {
        String originalUrl = params.getUrl().getSpec();
        String url = originalUrl.replaceFirst("^rewards://", "brave://rewards/");
        handler.clobberCurrentTabWithFallbackUrl(url, params);
    }
}
