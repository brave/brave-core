/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAdaptiveToolbarPrefsClassAdapter extends BraveClassVisitor {
    static String sAdaptiveToolbarPrefsClassName =
            "org/chromium/chrome/browser/toolbar/adaptive/AdaptiveToolbarPrefs";
    static String sBraveAdaptiveToolbarPrefsClassName =
            "org/chromium/chrome/browser/toolbar/adaptive/BraveAdaptiveToolbarPrefs";

    public BraveAdaptiveToolbarPrefsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sAdaptiveToolbarPrefsClassName,
                "getCustomizationSetting",
                sBraveAdaptiveToolbarPrefsClassName);

        changeMethodOwner(
                sAdaptiveToolbarPrefsClassName,
                "isCustomizationPreferenceEnabled",
                sBraveAdaptiveToolbarPrefsClassName);
    }
}
