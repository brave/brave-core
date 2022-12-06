/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStartupPaintPreviewHelperClassAdapter extends BraveClassVisitor {
    static String sStartupPaintPreviewHelperClassName =
            "org/chromium/chrome/browser/paint_preview/StartupPaintPreviewHelper";
    static String sBraveStartupPaintPreviewHelperClassName =
            "org/chromium/chrome/browser/paint_preview/BraveStartupPaintPreviewHelper";

    public BraveStartupPaintPreviewHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sStartupPaintPreviewHelperClassName, "isEnabled",
                sBraveStartupPaintPreviewHelperClassName);
    }
}
