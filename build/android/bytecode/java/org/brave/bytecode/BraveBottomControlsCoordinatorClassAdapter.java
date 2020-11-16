/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBottomControlsCoordinatorClassAdapter extends BraveClassVisitor {
    static String sBottomControlsCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/bottom/BottomControlsCoordinator";
    static String sBraveBottomControlsCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/bottom/BraveBottomControlsCoordinator";

    public BraveBottomControlsCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveBottomControlsCoordinatorClassName, "mMediator");
        makeProtectedField(sBottomControlsCoordinatorClassName, "mMediator");
    }
}
