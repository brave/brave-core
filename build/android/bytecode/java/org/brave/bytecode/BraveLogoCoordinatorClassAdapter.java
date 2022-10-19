/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLogoCoordinatorClassAdapter extends BraveClassVisitor {
    static String sLogoCoordinator = "org/chromium/chrome/browser/logo/LogoCoordinator";
    static String sBraveLogoCoordinator = "org/chromium/chrome/browser/logo/BraveLogoCoordinator";

    public BraveLogoCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sLogoCoordinator, sBraveLogoCoordinator);

        deleteField(sBraveLogoCoordinator, "mShouldShowLogo");
        makeProtectedField(sLogoCoordinator, "mShouldShowLogo");

        makePublicMethod(sLogoCoordinator, "updateVisibility");
        addMethodAnnotation(sBraveLogoCoordinator, "updateVisibility", "Ljava/lang/Override;");
    }
}
