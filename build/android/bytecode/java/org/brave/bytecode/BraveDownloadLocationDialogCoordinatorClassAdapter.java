/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDownloadLocationDialogCoordinatorClassAdapter extends BraveClassVisitor {
    static String sDownloadLocationDialogCoordinatorClassName =
            "org/chromium/chrome/browser/download/dialogs/DownloadLocationDialogCoordinator";
    static String sBraveDownloadLocationDialogCoordinatorClassName =
            "org/chromium/chrome/browser/download/dialogs/BraveDownloadLocationDialogCoordinator";

    public BraveDownloadLocationDialogCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sDownloadLocationDialogCoordinatorClassName,
                sBraveDownloadLocationDialogCoordinatorClassName);

        deleteField(sBraveDownloadLocationDialogCoordinatorClassName, "mController");
        makeProtectedField(sDownloadLocationDialogCoordinatorClassName, "mController");

        deleteMethod(
                sBraveDownloadLocationDialogCoordinatorClassName, "onDirectoryOptionsRetrieved");
        makePublicMethod(
                sDownloadLocationDialogCoordinatorClassName, "onDirectoryOptionsRetrieved");
    }
}
