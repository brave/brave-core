/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSettingsIntentUtilClassAdapter extends BraveClassVisitor {
    static String sSettingsIntentUtilClassName =
            "org/chromium/chrome/browser/settings/SettingsIntentUtil";

    static String sBraveSettingsIntentUtilClassName =
            "org/chromium/chrome/browser/settings/BraveSettingsIntentUtil";

    public BraveSettingsIntentUtilClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(
                sSettingsIntentUtilClassName, "createIntent", sBraveSettingsIntentUtilClassName);
    }
}
