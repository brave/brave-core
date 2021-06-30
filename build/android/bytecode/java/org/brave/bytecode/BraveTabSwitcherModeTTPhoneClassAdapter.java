/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabSwitcherModeTTPhoneClassAdapter extends BraveClassVisitor {
    static String sTabSwitcherModeTTPhoneClassName =
            "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTTPhone";
    static String sBraveTabSwitcherModeTTPhoneClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherModeTTPhone";

    public BraveTabSwitcherModeTTPhoneClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveTabSwitcherModeTTPhoneClassName, "mNewTabViewButton");
        makeProtectedField(sTabSwitcherModeTTPhoneClassName, "mNewTabViewButton");

        deleteField(sBraveTabSwitcherModeTTPhoneClassName, "mNewTabImageButton");
        makeProtectedField(sTabSwitcherModeTTPhoneClassName, "mNewTabImageButton");

        deleteField(sBraveTabSwitcherModeTTPhoneClassName, "mToggleTabStackButton");
        makeProtectedField(sTabSwitcherModeTTPhoneClassName, "mToggleTabStackButton");

        deleteField(sBraveTabSwitcherModeTTPhoneClassName, "mShouldShowNewTabVariation");
        makeProtectedField(sTabSwitcherModeTTPhoneClassName, "mShouldShowNewTabVariation");

        deleteField(sBraveTabSwitcherModeTTPhoneClassName, "mIsIncognito");
        makeProtectedField(sTabSwitcherModeTTPhoneClassName, "mIsIncognito");

        makePublicMethod(sTabSwitcherModeTTPhoneClassName, "updateNewTabButtonVisibility");
        addMethodAnnotation(sBraveTabSwitcherModeTTPhoneClassName, "updateNewTabButtonVisibility",
                "Ljava/lang/Override;");

        makePublicMethod(sTabSwitcherModeTTPhoneClassName, "getToolbarColorForCurrentState");
        addMethodAnnotation(sBraveTabSwitcherModeTTPhoneClassName, "getToolbarColorForCurrentState",
                "Ljava/lang/Override;");

        makePublicMethod(sTabSwitcherModeTTPhoneClassName, "shouldShowIncognitoToggle");
        deleteMethod(sBraveTabSwitcherModeTTPhoneClassName, "shouldShowIncognitoToggle");
    }
}
