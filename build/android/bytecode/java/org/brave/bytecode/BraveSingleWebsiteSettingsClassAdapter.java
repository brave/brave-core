/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSingleWebsiteSettingsClassAdapter extends BraveClassVisitor {
    static String sSingleWebsiteSettingsClassName = "org/chromium/components/browser_ui/site_settings/SingleWebsiteSettings";
    static String sBraveSingleWebsiteSettingsClassName = "org/chromium/components/browser_ui/site_settings/BraveSingleWebsiteSettings";

    public BraveSingleWebsiteSettingsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sSingleWebsiteSettingsClassName, sBraveSingleWebsiteSettingsClassName);

        changeMethodOwner(sSingleWebsiteSettingsClassName, "getPreferenceKey", sBraveSingleWebsiteSettingsClassName);
        changeMethodOwner(sSingleWebsiteSettingsClassName, "setupContentSettingsPreferences", sBraveSingleWebsiteSettingsClassName);

        makePublicMethod(sSingleWebsiteSettingsClassName, "setupContentSettingsPreference");
        changeMethodOwner(sBraveSingleWebsiteSettingsClassName, "setupContentSettingsPreference", sSingleWebsiteSettingsClassName);
    }
}
