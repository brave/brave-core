/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveToolbarSwipeLayoutClassAdapter extends BraveClassVisitor {
    static String sToolbarSwipeLayoutClassName =
            "org/chromium/chrome/browser/compositor/layouts/ToolbarSwipeLayout";
    static String sBraveToolbarSwipeLayoutClassName =
            "org/chromium/chrome/browser/compositor/layouts/BraveToolbarSwipeLayout";

    public BraveToolbarSwipeLayoutClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sToolbarSwipeLayoutClassName, sBraveToolbarSwipeLayoutClassName);

        deleteField(sBraveToolbarSwipeLayoutClassName, "mMoveToolbar");
        makeProtectedField(sToolbarSwipeLayoutClassName, "mMoveToolbar");
    }
}
