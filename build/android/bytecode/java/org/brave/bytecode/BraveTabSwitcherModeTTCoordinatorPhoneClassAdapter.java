/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabSwitcherModeTTCoordinatorPhoneClassAdapter extends BraveClassVisitor {
    static String sTabSwitcherModeTTCoordinatorPhoneClassName =
            "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTCoordinatorPhone";
    static String sBraveTabSwitcherModeTTCoordinatorPhoneClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherModeTTCoordinatorPhone";

    public BraveTabSwitcherModeTTCoordinatorPhoneClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveTabSwitcherModeTTCoordinatorPhoneClassName, "mTabSwitcherModeToolbar");
        makeProtectedField(sTabSwitcherModeTTCoordinatorPhoneClassName, "mTabSwitcherModeToolbar");
    }
}
