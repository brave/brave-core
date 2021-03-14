/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSettingsLauncherImplClassAdapter extends BraveClassVisitor {
    static String sSettingsLauncherImplClassName =
            "org/chromium/chrome/browser/settings/SettingsLauncherImpl";
    static String sBraveSettingsLauncherImplClassName =
            "org/chromium/chrome/browser/settings/BraveSettingsLauncherImpl";

    public BraveSettingsLauncherImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sSettingsLauncherImplClassName, sBraveSettingsLauncherImplClassName);
    }
}
