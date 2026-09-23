/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCachedFlagsClassAdapter extends BraveClassVisitor {
    static String sChromeCachedFlagsClassName =
            "org/chromium/chrome/browser/app/flags/ChromeCachedFlags";
    static String sBraveCachedFlagsClassName =
            "org/chromium/chrome/browser/app/flags/BraveCachedFlags";

    public BraveCachedFlagsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sChromeCachedFlagsClassName, sBraveCachedFlagsClassName);
    }
}
