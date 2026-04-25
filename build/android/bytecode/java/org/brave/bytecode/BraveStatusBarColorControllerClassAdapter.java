/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStatusBarColorControllerClassAdapter extends BraveClassVisitor {
    static String sStatusBarColorControllerClassName =
            "org/chromium/chrome/browser/ui/system/StatusBarColorController";
    static String sBraveStatusBarColorControllerClassName =
            "org/chromium/chrome/browser/ui/system/BraveStatusBarColorController";

    public BraveStatusBarColorControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sStatusBarColorControllerClassName, sBraveStatusBarColorControllerClassName);

        deleteField(sBraveStatusBarColorControllerClassName, "mBackgroundColorForNtp");
        makeProtectedField(sStatusBarColorControllerClassName, "mBackgroundColorForNtp");
    }
}
