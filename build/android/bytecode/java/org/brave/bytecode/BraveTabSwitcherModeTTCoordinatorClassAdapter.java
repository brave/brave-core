/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabSwitcherModeTTCoordinatorClassAdapter extends BraveClassVisitor {
    static String sTabSwitcherModeTTCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinator";
    static String sBraveTabSwitcherModeTTCoordinatorClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherModeTTCoordinator";

    public BraveTabSwitcherModeTTCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveTabSwitcherModeTTCoordinatorClassName, "mTabSwitcherModeToolbar");
        makeProtectedField(sTabSwitcherModeTTCoordinatorClassName, "mTabSwitcherModeToolbar");
    }
}
