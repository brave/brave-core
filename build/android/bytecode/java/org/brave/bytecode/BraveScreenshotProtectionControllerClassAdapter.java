/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveScreenshotProtectionControllerClassAdapter extends BraveClassVisitor {
    static String sScreenshotProtectionControllerClassName =
            "org/chromium/chrome/browser/screenshot_protection/ScreenshotProtectionController";
    static String sBraveScreenshotProtectionControllerClassName =
            "org/chromium/chrome/browser/screenshot_protection/BraveScreenshotProtectionController";

    public BraveScreenshotProtectionControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sScreenshotProtectionControllerClassName,
                sBraveScreenshotProtectionControllerClassName);
    }
}
