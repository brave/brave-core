/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNotificationPlatformBridgeClassAdapter extends BraveClassVisitor {
    static String sNotificationPlatformBridgeClassName =
            "org/chromium/chrome/browser/notifications/NotificationPlatformBridge";
    static String sBraveNotificationPlatformBridgeClassName =
            "org/chromium/chrome/browser/notifications/BraveNotificationPlatformBridge";

    public BraveNotificationPlatformBridgeClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sNotificationPlatformBridgeClassName,
                "dispatchNotificationEvent",
                sBraveNotificationPlatformBridgeClassName);

        changeMethodOwner(
                sNotificationPlatformBridgeClassName,
                "prepareNotificationBuilder",
                sBraveNotificationPlatformBridgeClassName);
    }
}
