/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNewTabPageLayoutClassAdapter extends BraveClassVisitor {
    static String sNewTabPageLayoutClassName = "org/chromium/chrome/browser/ntp/NewTabPageLayout";
    static String sBraveNewTabPageLayoutClassName =
            "org/chromium/chrome/browser/ntp/BraveNewTabPageLayout";
    static String sNewTabPageLayoutSuperClassName = "android/widget/FrameLayout";

    public BraveNewTabPageLayoutClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveNewTabPageLayoutClassName, "mMvTilesContainerLayout");
        makeProtectedField(sNewTabPageLayoutClassName, "mMvTilesContainerLayout");

        deleteField(sBraveNewTabPageLayoutClassName, "mLogoCoordinator");
        makeProtectedField(sNewTabPageLayoutClassName, "mLogoCoordinator");

        makePublicMethod(sNewTabPageLayoutClassName, "insertSiteSectionView");
        addMethodAnnotation(
                sBraveNewTabPageLayoutClassName, "insertSiteSectionView", "Ljava/lang/Override;");

        makePublicMethod(sNewTabPageLayoutClassName, "setSearchProviderTopMargin");
        addMethodAnnotation(
                sBraveNewTabPageLayoutClassName,
                "setSearchProviderTopMargin",
                "Ljava/lang/Override;");

        makePublicMethod(sNewTabPageLayoutClassName, "setSearchProviderBottomMargin");
        addMethodAnnotation(
                sBraveNewTabPageLayoutClassName,
                "setSearchProviderBottomMargin",
                "Ljava/lang/Override;");

        deleteMethod(sBraveNewTabPageLayoutClassName, "getLogoMargin");
        makePublicMethod(sNewTabPageLayoutClassName, "getLogoMargin");

        changeSuperName(sNewTabPageLayoutClassName, sNewTabPageLayoutSuperClassName);
    }
}
