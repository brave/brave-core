/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import org.chromium.base.annotations.NativeMethods;

public class BraveShieldsContentSettings {
    static public final String RESOURCE_IDENTIFIER_ADS = "ads";
    static public final String RESOURCE_IDENTIFIER_TRACKERS = "trackers";
    static public final String RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES = "httpUpgradableResources";
    static public final String RESOURCE_IDENTIFIER_BRAVE_SHIELDS = "braveShields";
    static public final String RESOURCE_IDENTIFIER_FINGERPRINTING = "fingerprinting";
    static public final String RESOURCE_IDENTIFIER_COOKIES = "cookies";
    static public final String RESOURCE_IDENTIFIER_REFERRERS = "referrers";
    static public final String RESOURCE_IDENTIFIER_JAVASCRIPTS = "javascript";

    static public void setShields(boolean incognito, String host, String resourceIndentifier, boolean value) {
        BraveShieldsContentSettingsJni.get().setShields(incognito, host, resourceIndentifier, value);
    }

    static public boolean getShields(boolean incognito, String host, String resourceIndentifier) {
        return BraveShieldsContentSettingsJni.get().getShields(incognito, host, resourceIndentifier); 
    }

    @NativeMethods
    interface Natives {
        void setShields(boolean incognito, String host, String resourceIndentifier, boolean value);
        boolean getShields(boolean incognito, String host, String resourceIndentifier);
    }
}
