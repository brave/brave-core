/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser;

// Used from org.chromium.chrome.browser.externalnav
public class BraveWalletProvider {
    public static final String ACTION_VALUE = "authorization";

    public static final String REDIRECT_URL_KEY = "redirect_url";

    public static final String UPHOLD_REDIRECT_URL = "rewards://uphold";
    public static final String GEMINI_REDIRECT_URL = "rewards://gemini";
    public static final String BITFLYER_REDIRECT_URL = "rewards://bitflyer";
    public static final String ZEBPAY_REDIRECT_URL = "rewards://zebpay";

    public static final String BRAVE_SUPPORT_URL = "https://community.brave.com";
    public static final String UPHOLD_ORIGIN_URL = "http://uphold.com";

    // Wallet types
    public static final String UPHOLD = "uphold";
    public static final String BITFLYER = "bitflyer";
    public static final String GEMINI = "gemini";
    public static final String ZEBPAY = "zebpay";
}
