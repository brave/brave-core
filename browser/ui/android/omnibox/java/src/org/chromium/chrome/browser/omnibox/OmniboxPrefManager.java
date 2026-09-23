/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;

import java.util.Calendar;
import java.util.Date;

public class OmniboxPrefManager {
    private static final String BRAVE_SEARCH_PROMO_BANNER_EXPIRED_DATE =
            "brave_search_promo_banner_expired_date";
    private static final String BRAVE_SEARCH_PROMO_BANNER_MAYBE_LATER =
            "brave_search_promo_banner_maybe_later";
    private static final String BRAVE_SEARCH_PROMO_BANNER_DISMISSED =
            "brave_search_promo_banner_dismissed";

    private static OmniboxPrefManager sInstance;
    private final SharedPreferences mSharedPreferences;

    private boolean isBraveSearchPromoBannerDismissedCurrentSession;

    private OmniboxPrefManager() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    /**
     * Returns the singleton instance of OmniboxPrefManager, creating it if needed.
     */
    public static OmniboxPrefManager getInstance() {
        if (sInstance == null) {
            sInstance = new OmniboxPrefManager();
        }
        return sInstance;
    }

    public long getBraveSearchPromoBannerExpiredDate() {
        return mSharedPreferences.getLong(BRAVE_SEARCH_PROMO_BANNER_EXPIRED_DATE, 0);
    }

    public void setBraveSearchPromoBannerExpiredDate() {
        Calendar calender = Calendar.getInstance();
        calender.setTime(new Date());
        calender.add(Calendar.DATE, 14);

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(
                BRAVE_SEARCH_PROMO_BANNER_EXPIRED_DATE, calender.getTimeInMillis());
        sharedPreferencesEditor.apply();
    }

    public boolean isBraveSearchPromoBannerMaybeLater() {
        return mSharedPreferences.getBoolean(BRAVE_SEARCH_PROMO_BANNER_MAYBE_LATER, false);
    }

    public void setBraveSearchPromoBannerMaybeLater() {
        isBraveSearchPromoBannerDismissedCurrentSession = true;

        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(BRAVE_SEARCH_PROMO_BANNER_MAYBE_LATER, true);
        sharedPreferencesEditor.apply();
    }

    public boolean isBraveSearchPromoBannerDismissed() {
        return mSharedPreferences.getBoolean(BRAVE_SEARCH_PROMO_BANNER_DISMISSED, false);
    }

    public void setBraveSearchPromoBannerDismissed() {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(BRAVE_SEARCH_PROMO_BANNER_DISMISSED, true);
        sharedPreferencesEditor.apply();
    }

    public boolean isBraveSearchPromoBannerDismissedCurrentSession() {
        return isBraveSearchPromoBannerDismissedCurrentSession;
    }
}
