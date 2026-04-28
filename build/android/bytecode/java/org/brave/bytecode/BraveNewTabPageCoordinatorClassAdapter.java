/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNewTabPageCoordinatorClassAdapter extends BraveClassVisitor {
    static String sNewTabPageCoordinatorClassName =
            "org/chromium/chrome/browser/ntp/NewTabPageCoordinator";
    static String sBraveNewTabPageCoordinatorClassName =
            "org/chromium/chrome/browser/ntp/BraveNewTabPageCoordinator";

    public BraveNewTabPageCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sNewTabPageCoordinatorClassName, sBraveNewTabPageCoordinatorClassName);
    }
}
