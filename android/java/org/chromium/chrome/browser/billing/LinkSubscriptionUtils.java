/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.billing;

import org.chromium.chrome.browser.billing.InAppPurchaseWrapper.SubscriptionProduct;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.util.Locale;

public class LinkSubscriptionUtils {
    private static final String BRAVE_ACCOUNT_URL_STAGING = "account.bravesoftware.com";
    private static final String BRAVE_ACCOUNT_URL = "account.brave.com";

    public static final String PREF_LINK_SUBSCRIPTION_ON_STAGING = "link_subscription_on_staging";

    public static String getBraveAccountLinkUrl(SubscriptionProduct subscriptionProduct) {
        String braveAccountUrl =
                isLinkSubscriptionOnStaging() ? BRAVE_ACCOUNT_URL_STAGING : BRAVE_ACCOUNT_URL;
        String linkType =
                (SubscriptionProduct.LEO == subscriptionProduct) ? "link-order" : "connect-receipt";
        String baseUrl = "https://%s?intent=%s&product=%s";
        return String.format(
                baseUrl,
                braveAccountUrl,
                linkType,
                subscriptionProduct.name().toLowerCase(Locale.ROOT));
    }

    public static String getBraveAccountRecoverUrl(SubscriptionProduct subscriptionProduct) {
        String braveAccountUrl =
                isLinkSubscriptionOnStaging() ? BRAVE_ACCOUNT_URL_STAGING : BRAVE_ACCOUNT_URL;
        String baseUrl = "https://%s?intent=recover&product=%s&ux=mobile";
        return String.format(
                baseUrl, braveAccountUrl, subscriptionProduct.name().toLowerCase(Locale.ROOT));
    }

    public static boolean isLinkSubscriptionOnStaging() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(PREF_LINK_SUBSCRIPTION_ON_STAGING, false);
    }
}
