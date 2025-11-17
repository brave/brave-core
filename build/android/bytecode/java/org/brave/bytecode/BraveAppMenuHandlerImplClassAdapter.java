/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAppMenuHandlerImplClassAdapter extends BraveClassVisitor {
    static String sAppMenuHandlerImpl = "org/chromium/chrome/browser/ui/appmenu/AppMenuHandlerImpl";
    static String sBraveAppMenuHandlerImpl =
            "org/chromium/chrome/browser/ui/appmenu/BraveAppMenuHandlerImpl";

    public BraveAppMenuHandlerImplClassAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(sAppMenuHandlerImpl, sBraveAppMenuHandlerImpl);

        deleteField(sBraveAppMenuHandlerImpl, "mAppMenu");
        makeProtectedField(sAppMenuHandlerImpl, "mAppMenu");

        deleteField(sBraveAppMenuHandlerImpl, "mAppMenuDragHelper");
        makeProtectedField(sAppMenuHandlerImpl, "mAppMenuDragHelper");
    }
}
