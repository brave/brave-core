/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveQuickActionSearchWidgetProviderClassAdapter extends BraveClassVisitor {
    static String sQuickActionSearchWidgetProviderClassName =
            "org/chromium/chrome/browser/quickactionsearchwidget/QuickActionSearchWidgetProvider";
    static String sBraveQuickActionSearchWidgetProviderClassName =
            "org/chromium/chrome/browser/quickactionsearchwidget/BraveQuickActionSearchWidgetProvider";

    public BraveQuickActionSearchWidgetProviderClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(sQuickActionSearchWidgetProviderClassName, "setWidgetEnabled",
                sBraveQuickActionSearchWidgetProviderClassName);

        deleteMethod(sBraveQuickActionSearchWidgetProviderClassName, "setWidgetComponentEnabled");
        makePublicMethod(sQuickActionSearchWidgetProviderClassName, "setWidgetComponentEnabled");
    }
}
