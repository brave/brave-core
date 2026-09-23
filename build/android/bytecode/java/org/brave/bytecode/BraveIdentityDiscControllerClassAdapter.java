/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIdentityDiscControllerClassAdapter extends BraveClassVisitor {
    static String sIdentityDiscControllerClassName =
            "org/chromium/chrome/browser/identity_disc/IdentityDiscController";
    static String sBraveIdentityDiscControllerClassName =
            "org/chromium/chrome/browser/identity_disc/BraveIdentityDiscController";

    public BraveIdentityDiscControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sIdentityDiscControllerClassName, sBraveIdentityDiscControllerClassName);

        makePublicMethod(sIdentityDiscControllerClassName, "calculateButtonData");
        addMethodAnnotation(
                sBraveIdentityDiscControllerClassName,
                "calculateButtonData",
                "Ljava/lang/Override;");
    }
}
