/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.user_prefs.UserPrefs;

public class BravePrivacySettingsIPFSUtils {
    public static void setIPFSGatewayPref(boolean preference) {
        if (BraveConfig.IPFS_ENABLED) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setInteger(BravePref.IPFS_RESOLVE_METHOD,
                            preference ? IPFSResolveMethodTypes.IPFS_ASK
                                       : IPFSResolveMethodTypes.IPFS_DISABLED);
        }
    }

    public static boolean getIPFSGatewayPref() {
        if (BraveConfig.IPFS_ENABLED) {
            return UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                           .getInteger(BravePref.IPFS_RESOLVE_METHOD)
                    != IPFSResolveMethodTypes.IPFS_DISABLED;
        } else {
            return false;
        }
    }
}
