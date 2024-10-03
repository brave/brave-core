/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabbedNavigationBarColorControllerBaseClassAdapter extends BraveClassVisitor {
    static String sTabbedNavigationBarColorControllerClassName =
            "org/chromium/chrome/browser/tabbed_mode/TabbedNavigationBarColorController";
    static String sBraveTabbedNavigationBarColorControllerBaseClassName =
            "org/chromium/chrome/browser/tabbed_mode/BraveTabbedNavigationBarColorControllerBase";

    public BraveTabbedNavigationBarColorControllerBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(
                sTabbedNavigationBarColorControllerClassName,
                sBraveTabbedNavigationBarColorControllerBaseClassName);

        deleteField(sTabbedNavigationBarColorControllerClassName, "mContext");

        changeMethodOwner(
                sTabbedNavigationBarColorControllerClassName,
                "getNavigationBarColor",
                sBraveTabbedNavigationBarColorControllerBaseClassName);
    }
}
