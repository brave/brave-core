/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSnackbarManagerClassAdapter extends BraveClassVisitor {
    static String sSnackbarManagerClassName =
            "org/chromium/chrome/browser/ui/messages/snackbar/SnackbarManager";
    static String sBraveSnackbarManagerClassName =
            "org/chromium/chrome/browser/ui/messages/snackbar/BraveSnackbarManager";

    public BraveSnackbarManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveSnackbarManagerClassName, "mView");
        makeProtectedField(sSnackbarManagerClassName, "mView");
    }
}
