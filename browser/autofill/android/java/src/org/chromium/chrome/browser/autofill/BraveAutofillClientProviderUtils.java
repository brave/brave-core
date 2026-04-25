/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import org.jni_zero.CalledByNative;
import org.jni_zero.JniType;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.components.prefs.PrefService;

/**
 * Brave override for {@link AutofillClientProviderUtils}.
 *
 * <p>Unlike Chrome, Brave does not have a built-in Google-synced autofill, so Google's Autofill
 * with Google (AWG) service should be treated like any other third-party autofill provider (e.g.,
 * 1Password, Bitwarden). This override remaps {@code ANDROID_AUTOFILL_SERVICE_IS_GOOGLE} to either
 * {@code AVAILABLE} or {@code SETTING_TURNED_OFF}, enabling the autofill toggle in settings.
 */
@NullMarked
public class BraveAutofillClientProviderUtils {
    @CalledByNative
    public static int getAndroidAutofillFrameworkAvailability(
            @JniType("PrefService*") PrefService prefs) {
        int result = AutofillClientProviderUtils.getAndroidAutofillFrameworkAvailability(prefs);
        if (result == AndroidAutofillAvailabilityStatus.ANDROID_AUTOFILL_SERVICE_IS_GOOGLE) {
            if (prefs.getBoolean(Pref.AUTOFILL_USING_PLATFORM_AUTOFILL)) {
                return AndroidAutofillAvailabilityStatus.AVAILABLE;
            }
            return AndroidAutofillAvailabilityStatus.SETTING_TURNED_OFF;
        }
        return result;
    }
}
