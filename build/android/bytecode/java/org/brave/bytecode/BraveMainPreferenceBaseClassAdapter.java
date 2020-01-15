/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMainPreferenceBaseClassAdapter extends BraveClassVisitor {

    static String sMainPreferencesClassName =
            "org/chromium/chrome/browser/preferences/MainPreferences";

    static String sBraveMainPreferencesBaseClassName =
            "org/chromium/chrome/browser/preferences/BraveMainPreferencesBase";

    public BraveMainPreferenceBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sMainPreferencesClassName,
                        sBraveMainPreferencesBaseClassName);
    }
}
