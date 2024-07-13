/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTopToolbarCoordinatorClassAdapter extends BraveClassVisitor {
    static String sTopToolbarCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/top/TopToolbarCoordinator";
    static String sBraveTopToolbarCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveTopToolbarCoordinator";

    public BraveTopToolbarCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sTopToolbarCoordinatorClassName, sBraveTopToolbarCoordinatorClassName);

        deleteField(sBraveTopToolbarCoordinatorClassName, "mOptionalButtonController");
        makeProtectedField(sTopToolbarCoordinatorClassName, "mOptionalButtonController");

        deleteField(sBraveTopToolbarCoordinatorClassName, "mToolbarColorObserverManager");
        makeProtectedField(sTopToolbarCoordinatorClassName, "mToolbarColorObserverManager");
    }
}
