/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAdsNotificationBuilderClassAdapter extends BraveClassVisitor {
    static String sStandardNotificationBuilderClassName =
            "org/chromium/chrome/browser/notifications/StandardNotificationBuilder";
    static String sBraveAdsNotificationBuilderClassName =
            "org/chromium/chrome/browser/notifications/BraveAdsNotificationBuilder";

    public BraveAdsNotificationBuilderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sStandardNotificationBuilderClassName, sBraveAdsNotificationBuilderClassName);
    }
}
