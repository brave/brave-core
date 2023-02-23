/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNotificationBuilderClassAdapter extends BraveClassVisitor {
    static String sStandardNotificationBuilderClassName =
            "org/chromium/chrome/browser/notifications/StandardNotificationBuilder";
    static String sBraveNotificationBuilderClassName =
            "org/chromium/chrome/browser/notifications/BraveNotificationBuilder";

    public BraveNotificationBuilderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sStandardNotificationBuilderClassName, sBraveNotificationBuilderClassName);
    }
}
