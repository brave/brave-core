/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveThemeUtilsClassAdapter extends BraveClassVisitor {
    static String sThemeUtilsClassName = "org/chromium/chrome/browser/theme/ThemeUtils";
    static String sBraveThemeUtilsClassName = "org/chromium/chrome/browser/theme/BraveThemeUtils";

    public BraveThemeUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sThemeUtilsClassName,
                "getTextBoxColorForToolbarBackgroundInNonNativePage", sBraveThemeUtilsClassName);
    }
}
