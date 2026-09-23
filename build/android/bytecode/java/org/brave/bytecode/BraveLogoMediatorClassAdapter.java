/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLogoMediatorClassAdapter extends BraveClassVisitor {
    static String sLogoMediator = "org/chromium/chrome/browser/logo/LogoMediator";
    static String sBraveLogoMediator = "org/chromium/chrome/browser/logo/BraveLogoMediator";

    public BraveLogoMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sLogoMediator, sBraveLogoMediator);

        deleteField(sBraveLogoMediator, "mLogoModel");
        makeProtectedField(sLogoMediator, "mLogoModel");

        deleteField(sBraveLogoMediator, "mShouldShowLogo");
        makeProtectedField(sLogoMediator, "mShouldShowLogo");

        makePublicMethod(sLogoMediator, "updateVisibility");
        addMethodAnnotation(sBraveLogoMediator, "updateVisibility", "Ljava/lang/Override;");
    }
}
