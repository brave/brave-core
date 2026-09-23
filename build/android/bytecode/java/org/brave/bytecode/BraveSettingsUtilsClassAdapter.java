/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSettingsUtilsClassAdapter extends BraveClassVisitor {
    static String sSettingsUtilsClassName =
            "org/chromium/components/browser_ui/settings/SettingsUtils";

    static String sBraveSettingsUtilsClassName =
            "org/chromium/components/browser_ui/settings/BraveSettingsUtils";

    public BraveSettingsUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(
                sSettingsUtilsClassName, "getVisiblePreferences", sBraveSettingsUtilsClassName);
    }
}
