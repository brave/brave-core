/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabbedRootUiCoordinatorClassAdapter extends BraveClassVisitor {
    static String sTabbedRootUiCoordinatorClassName =
            "org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator";
    static String sBraveTabbedRootUiCoordinatorClassName =
            "org/chromium/chrome/browser/tabbed_mode/BraveTabbedRootUiCoordinator";

    public BraveTabbedRootUiCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sTabbedRootUiCoordinatorClassName, sBraveTabbedRootUiCoordinatorClassName);
    }
}
