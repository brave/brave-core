/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLayoutManagerChromeClassAdapter extends BraveClassVisitor {
    static String sLayoutManagerChromePhoneClassName =
            "org/chromium/chrome/browser/compositor/layouts/LayoutManagerChromePhone";

    static String sBraveLayoutManagerChromeClassName =
            "org/chromium/chrome/browser/compositor/layouts/BraveLayoutManagerChrome";

    public BraveLayoutManagerChromeClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sLayoutManagerChromePhoneClassName, sBraveLayoutManagerChromeClassName);
    }
}
