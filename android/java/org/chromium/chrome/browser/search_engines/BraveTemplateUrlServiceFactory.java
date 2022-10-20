/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.search_engines.TemplateUrlService;

import java.lang.reflect.InvocationTargetException;

public class BraveTemplateUrlServiceFactory {
    private static final String TAG = "BraveTemplateUrlServiceFactory";

    private static TemplateUrlService sNormalTemplateUrlService;
    private static @Nullable Profile sProfile;

    private BraveTemplateUrlServiceFactory() {}

    /**
     * @return The singleton instance of {@link TemplateUrlService} for profile, based on current
     *         active tab model.
     * All TemplateUrlServiceFactory.get() calls are redirected here.
     */
    public static TemplateUrlService get() {
        ThreadUtils.assertOnUiThread();

        if (sProfile != null) return getForProfile(sProfile);

        return BraveTemplateUrlServiceFactoryJni.get().getTemplateUrlService();
    }

    /**
     * @return The singleton instance of {@link TemplateUrlService} for each profile, creating it if
     *         necessary.
     */
    public static TemplateUrlService getForProfile(Profile profile) {
        ThreadUtils.assertOnUiThread();

        if (!profile.isOffTheRecord()) {
            if (sNormalTemplateUrlService == null) {
                sNormalTemplateUrlService =
                        BraveTemplateUrlServiceFactoryJni.get().getTemplateUrlServiceByProfile(
                                profile);
            }
            return sNormalTemplateUrlService;
        } else {
            // Do not cache OTR service. It will be destroyed every time
            // a new private tab model is created
            return BraveTemplateUrlServiceFactoryJni.get().getTemplateUrlServiceByProfile(profile);
        }
    }

    @VisibleForTesting
    public static void setInstanceForTesting(
            TemplateUrlService normalService, TemplateUrlService otrService) {
        sNormalTemplateUrlService = normalService;
    }

    public static void setCurrentProfile(@Nullable Profile profile) {
        sProfile = profile;
    }

    // Natives interface is public to allow mocking in tests outside of
    // org.chromium.chrome.browser.search_engines package.
    @NativeMethods
    public interface Natives {
        TemplateUrlService getTemplateUrlService();
        TemplateUrlService getTemplateUrlServiceByProfile(Profile profile);
    }
}
