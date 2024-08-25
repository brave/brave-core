/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIncognitoTabSwitcherPaneClassAdapter extends BraveClassVisitor {
    static String sIncognitoTabSwitcherPaneClassName =
            "org/chromium/chrome/browser/tasks/tab_management/IncognitoTabSwitcherPane";

    static String sBraveIncognitoTabSwitcherPaneClassName =
            "org/chromium/chrome/browser/tasks/tab_management/BraveIncognitoTabSwitcherPane";

    public BraveIncognitoTabSwitcherPaneClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sIncognitoTabSwitcherPaneClassName, sBraveIncognitoTabSwitcherPaneClassName);
    }
}
