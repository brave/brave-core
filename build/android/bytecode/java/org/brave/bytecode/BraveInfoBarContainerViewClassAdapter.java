/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveInfoBarContainerViewClassAdapter extends BraveClassVisitor {
    static String sInfoBarContainerViewClassName =
            "org/chromium/chrome/browser/infobar/InfoBarContainerView";

    static String sBraveInfoBarContainerViewClassName =
            "org/chromium/chrome/browser/infobar/BraveInfoBarContainerView";

    public BraveInfoBarContainerViewClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveInfoBarContainerViewClassName, "mEdgeToEdgeSupplier");
        makeProtectedField(sInfoBarContainerViewClassName, "mEdgeToEdgeSupplier");
        redirectConstructor(sInfoBarContainerViewClassName, sBraveInfoBarContainerViewClassName);
    }
}
