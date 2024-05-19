/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.language;

import android.app.LocaleManager;
import android.os.Build;
import android.os.LocaleList;
import android.text.TextUtils;

import androidx.annotation.RequiresApi;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

import java.util.Locale;

public class BraveLocaleManagerDelegateImpl extends LocaleManagerDelegateImpl {
    private static final String TAG = "in_app_language";

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    @Override
    public Locale getApplicationLocale() {
        LocaleList currentAppLocales =
                ContextUtils.getApplicationContext()
                        .getSystemService(LocaleManager.class)
                        .getApplicationLocales();
        if (!currentAppLocales.isEmpty()) {
            return currentAppLocales.get(0);
        }
        return null;
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    @Override
    public void setApplicationLocale(String languageName) {
        try {
            if (languageName != null && !TextUtils.isEmpty(languageName)) {
                ContextUtils.getApplicationContext()
                        .getSystemService(LocaleManager.class)
                        .setApplicationLocales(new LocaleList(Locale.forLanguageTag(languageName)));
            }
        } catch (Exception ex) {
            Log.e(TAG, "setApplicationLocale : " + ex.getMessage());
        }
    }
}
