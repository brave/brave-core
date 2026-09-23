/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNotificationPermissionRationaleDialogControllerClassAdapter
        extends BraveClassVisitor {
    static String sContoller =
            "org/chromium/chrome/browser/notifications/permissions/NotificationPermissionRationaleDialogController";
    static String sBraveContoller =
            "org/chromium/chrome/browser/notifications/permissions/BraveNotificationPermissionRationaleDialogController";

    public BraveNotificationPermissionRationaleDialogControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sContoller, sBraveContoller);
    }
}
