/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStandardProtectionSettingsFragmentClassAdapter extends BraveClassVisitor {
    static String sStandardProtectionSettingsFragmentClassName =
            "org/chromium/chrome/browser/safe_browsing/settings/StandardProtectionSettingsFragment";
    static String sBraveStandardProtectionSettingsFragmentClassName =
            "org/chromium/chrome/browser/safe_browsing/settings/BraveStandardProtectionSettingsFragment";

    public BraveStandardProtectionSettingsFragmentClassAdapter(ClassVisitor visitor) {
        super(visitor);
        makePublicMethod(sStandardProtectionSettingsFragmentClassName,
                "updateLeakDetectionAndExtendedReportingPreferences");
        changeMethodOwner(sStandardProtectionSettingsFragmentClassName,
                "updateLeakDetectionAndExtendedReportingPreferences",
                sBraveStandardProtectionSettingsFragmentClassName);
    }
}
