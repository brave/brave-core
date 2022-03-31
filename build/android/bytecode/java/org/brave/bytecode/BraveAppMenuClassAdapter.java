/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAppMenuClassAdapter extends BraveClassVisitor {
    static String sAppMenuClassName = "org/chromium/chrome/browser/ui/appmenu/AppMenu";

    static String sBraveAppMenuClassName = "org/chromium/chrome/browser/ui/appmenu/BraveAppMenu";

    public BraveAppMenuClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sAppMenuClassName, sBraveAppMenuClassName);

        changeMethodOwner(sAppMenuClassName, "getPopupPosition", sBraveAppMenuClassName);

        makePublicMethod(sAppMenuClassName, "runMenuItemEnterAnimations");
        addMethodAnnotation(
                sBraveAppMenuClassName, "runMenuItemEnterAnimations", "Ljava/lang/Override;");
    }
}
