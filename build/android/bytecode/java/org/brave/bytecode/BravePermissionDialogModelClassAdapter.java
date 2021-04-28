/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePermissionDialogModelClassAdapter extends BraveClassVisitor {
    static String sPermissionDialogModelClassName =
            "org/chromium/components/permissions/PermissionDialogModel";

    static String sBravePermissionDialogModelClassName =
            "org/chromium/components/permissions/BravePermissionDialogModel";

    public BravePermissionDialogModelClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(
                sPermissionDialogModelClassName, "getModel", sBravePermissionDialogModelClassName);
    }
}
