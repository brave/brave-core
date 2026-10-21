/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;

@NullMarked
public class BraveProfileUtils {
    /**
     * Runs {@code callback} with the regular profile: right away when it already exists, otherwise
     * once it is created.
     *
     * <p>Replaces upstream's OriginalProfileSupplier, removed as unused in
     * https://crrev.com/a1ca906126779. Unlike that class, this handles the case where the profile
     * already exists, which ProfileManager observers are never notified about.
     */
    public static void onRegularProfileAvailable(Callback<Profile> callback) {
        ThreadUtils.assertOnUiThread();
        if (ProfileManager.isInitialized()) {
            callback.onResult(ProfileManager.getLastUsedRegularProfile());
            return;
        }
        ProfileManager.addObserver(
                new ProfileManager.Observer() {
                    @Override
                    public void onProfileAdded(Profile profile) {
                        if (profile.isOffTheRecord()) {
                            return;
                        }
                        ProfileManager.removeObserver(this);
                        callback.onResult(profile);
                    }

                    @Override
                    public void onProfileDestroyed(Profile profile) {}
                });
    }
}
