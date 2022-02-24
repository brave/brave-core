/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabSwitcherModeTopToolbarClassAdapter extends BraveClassVisitor {
    static String sTabSwitcherModeTopToolbarClassName =
            "org/chromium/chrome/browser/toolbar/top/TabSwitcherModeTopToolbar";
    static String sBraveTabSwitcherModeTopToolbarClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherModeTopToolbar";

    public BraveTabSwitcherModeTopToolbarClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveTabSwitcherModeTopToolbarClassName, "mNewTabViewButton");
        makeProtectedField(sTabSwitcherModeTopToolbarClassName, "mNewTabViewButton");

        deleteField(sBraveTabSwitcherModeTopToolbarClassName, "mNewTabImageButton");
        makeProtectedField(sTabSwitcherModeTopToolbarClassName, "mNewTabImageButton");

        deleteField(sBraveTabSwitcherModeTopToolbarClassName, "mToggleTabStackButton");
        makeProtectedField(sTabSwitcherModeTopToolbarClassName, "mToggleTabStackButton");

        deleteField(sBraveTabSwitcherModeTopToolbarClassName, "mShouldShowNewTabVariation");
        makeProtectedField(sTabSwitcherModeTopToolbarClassName, "mShouldShowNewTabVariation");

        deleteField(sBraveTabSwitcherModeTopToolbarClassName, "mIsIncognito");
        makeProtectedField(sTabSwitcherModeTopToolbarClassName, "mIsIncognito");

        makePublicMethod(sTabSwitcherModeTopToolbarClassName, "updateNewTabButtonVisibility");
        addMethodAnnotation(sBraveTabSwitcherModeTopToolbarClassName,
                "updateNewTabButtonVisibility", "Ljava/lang/Override;");

        makePublicMethod(sTabSwitcherModeTopToolbarClassName, "getToolbarColorForCurrentState");
        addMethodAnnotation(sBraveTabSwitcherModeTopToolbarClassName,
                "getToolbarColorForCurrentState", "Ljava/lang/Override;");

        makePublicMethod(sTabSwitcherModeTopToolbarClassName, "shouldShowIncognitoToggle");
        deleteMethod(sBraveTabSwitcherModeTopToolbarClassName, "shouldShowIncognitoToggle");
    }
}
