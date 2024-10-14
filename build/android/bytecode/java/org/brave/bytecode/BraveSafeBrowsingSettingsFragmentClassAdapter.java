/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSafeBrowsingSettingsFragmentClassAdapter extends BraveClassVisitor {
    static String sSafeBrowsingSettingsFragmentClassName =
            "org/chromium/chrome/browser/safe_browsing/settings/SafeBrowsingSettingsFragment";

    static String sBraveSafeBrowsingSettingsFragmentClassName =
            "org/chromium/chrome/browser/safe_browsing/settings/BraveSafeBrowsingSettingsFragment";

    public BraveSafeBrowsingSettingsFragmentClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(
                sSafeBrowsingSettingsFragmentClassName,
                "getSafeBrowsingSummaryString",
                sBraveSafeBrowsingSettingsFragmentClassName);
    }
}
