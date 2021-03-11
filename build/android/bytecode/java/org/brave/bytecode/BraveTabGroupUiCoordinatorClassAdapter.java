/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabGroupUiCoordinatorClassAdapter extends BraveClassVisitor {
    static String sTabGroupUiCoordinatorClassName =
            "org/chromium/chrome/browser/tasks/tab_management/TabGroupUiCoordinator";
    static String sBraveTabGroupUiCoordinatorClassName =
            "org/chromium/chrome/browser/tasks/tab_management/BraveTabGroupUiCoordinator";

    public BraveTabGroupUiCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sTabGroupUiCoordinatorClassName, sBraveTabGroupUiCoordinatorClassName);

        deleteField(sBraveTabGroupUiCoordinatorClassName, "mToolbarView");
        makeProtectedField(sTabGroupUiCoordinatorClassName, "mToolbarView");
    }
}
