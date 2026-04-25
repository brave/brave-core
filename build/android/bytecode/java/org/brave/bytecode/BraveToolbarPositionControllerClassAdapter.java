/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveToolbarPositionControllerClassAdapter extends BraveClassVisitor {
    static String sToolbarPositionControllerClassName =
            "org/chromium/chrome/browser/toolbar/ToolbarPositionController";
    static String sBraveToolbarPositionControllerClassName =
            "org/chromium/chrome/browser/toolbar/BraveToolbarPositionController";

    public BraveToolbarPositionControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sToolbarPositionControllerClassName,
                "calculateStateTransition",
                sBraveToolbarPositionControllerClassName);
    }
}
