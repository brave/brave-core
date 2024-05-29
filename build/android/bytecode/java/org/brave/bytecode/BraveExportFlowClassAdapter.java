/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveExportFlowClassAdapter extends BraveClassVisitor {
    static String sExportFlowClassName =
            "org/chromium/chrome/browser/password_manager/settings/ExportFlow";
    static String sBraveExportFlowClassName =
            "org/chromium/chrome/browser/password_manager/settings/BraveExportFlow";

    public BraveExportFlowClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sExportFlowClassName, sBraveExportFlowClassName);

        makePublicMethod(sExportFlowClassName, "runSharePasswordsIntent");
        addMethodAnnotation(
                sBraveExportFlowClassName, "runSharePasswordsIntent", "Ljava/lang/Override;");
    }
}
