/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveForegroundServiceUtilsClassAdapter extends BraveClassVisitor {
    static String sForegroundServiceUtilsClassName =
            "org/chromium/components/browser_ui/notifications/ForegroundServiceUtils";

    static String sBraveForegroundServiceUtilsClassName =
            "org/chromium/components/browser_ui/notifications/BraveForegroundServiceUtils";

    public BraveForegroundServiceUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sForegroundServiceUtilsClassName, sBraveForegroundServiceUtilsClassName);
    }
}
