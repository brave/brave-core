/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDynamicColorsClassAdapter extends BraveClassVisitor {
    static String sDynamicColorsClassName = "com/google/android/material/color/DynamicColors";

    static String sBraveDynamicColorsClassName =
            "org/chromium/chrome/browser/util/BraveDynamicColors";

    public BraveDynamicColorsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sDynamicColorsClassName, "applyToActivityIfAvailable",
                sBraveDynamicColorsClassName);
    }
}
