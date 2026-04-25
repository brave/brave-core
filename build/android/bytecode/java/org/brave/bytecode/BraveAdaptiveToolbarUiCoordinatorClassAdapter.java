/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAdaptiveToolbarUiCoordinatorClassAdapter extends BraveClassVisitor {
    static String sAdaptiveToolbarUiCoordinator =
            "org/chromium/chrome/browser/ui/AdaptiveToolbarUiCoordinator";
    static String sBraveAdaptiveToolbarUiCoordinator =
            "org/chromium/chrome/browser/ui/BraveAdaptiveToolbarUiCoordinator";

    public BraveAdaptiveToolbarUiCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sAdaptiveToolbarUiCoordinator, sBraveAdaptiveToolbarUiCoordinator);

        deleteField(sBraveAdaptiveToolbarUiCoordinator, "mContext");
        makeProtectedField(sAdaptiveToolbarUiCoordinator, "mContext");

        deleteField(sBraveAdaptiveToolbarUiCoordinator, "mActivityTabProvider");
        makeProtectedField(sAdaptiveToolbarUiCoordinator, "mActivityTabProvider");

        deleteField(sBraveAdaptiveToolbarUiCoordinator, "mModalDialogManagerSupplier");
        makeProtectedField(sAdaptiveToolbarUiCoordinator, "mModalDialogManagerSupplier");

        deleteField(sBraveAdaptiveToolbarUiCoordinator, "mProfileSupplier");
        makeProtectedField(sAdaptiveToolbarUiCoordinator, "mProfileSupplier");

        deleteField(sBraveAdaptiveToolbarUiCoordinator, "mAdaptiveToolbarButtonController");
        makeProtectedField(sAdaptiveToolbarUiCoordinator, "mAdaptiveToolbarButtonController");
    }
}
