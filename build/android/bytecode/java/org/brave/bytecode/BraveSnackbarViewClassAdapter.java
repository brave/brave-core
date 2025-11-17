/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSnackbarViewClassAdapter extends BraveClassVisitor {
    static String sBraveSnackbarViewClassName =
            "org/chromium/chrome/browser/ui/messages/snackbar/BraveSnackbarView";
    static String sSnackbarViewClassName =
            "org/chromium/chrome/browser/ui/messages/snackbar/SnackbarView";

    public BraveSnackbarViewClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sSnackbarViewClassName, sBraveSnackbarViewClassName);
        deleteField(sBraveSnackbarViewClassName, "mContainerView");
        makeProtectedField(sSnackbarViewClassName, "mContainerView");
    }
}
