/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNewTabPageClassAdapter extends BraveClassVisitor {
    static String sNewTabPageClassName = "org/chromium/chrome/browser/ntp/NewTabPage";
    static String sBraveNewTabPageClassName = "org/chromium/chrome/browser/ntp/BraveNewTabPage";

    public BraveNewTabPageClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sNewTabPageClassName, sBraveNewTabPageClassName);

        deleteField(sBraveNewTabPageClassName, "mBrowserControlsStateProvider");
        makeProtectedField(sNewTabPageClassName, "mBrowserControlsStateProvider");

        deleteField(sBraveNewTabPageClassName, "mNewTabPageLayout");
        makeProtectedField(sNewTabPageClassName, "mNewTabPageLayout");

        deleteField(sBraveNewTabPageClassName, "mFeedSurfaceProvider");
        makeProtectedField(sNewTabPageClassName, "mFeedSurfaceProvider");

        deleteField(sBraveNewTabPageClassName, "mToolbarSupplier");
        makeProtectedField(sNewTabPageClassName, "mToolbarSupplier");

        deleteField(sBraveNewTabPageClassName, "mBottomSheetController");
        makeProtectedField(sNewTabPageClassName, "mBottomSheetController");

        makePublicMethod(sNewTabPageClassName, "updateSearchProviderHasLogo");
        addMethodAnnotation(
                sBraveNewTabPageClassName, "updateSearchProviderHasLogo", "Ljava/lang/Override;");
    }
}
