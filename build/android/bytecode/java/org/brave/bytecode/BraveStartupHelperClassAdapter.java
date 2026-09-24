/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStartupHelperClassAdapter extends BraveClassVisitor {
    static String sStartupHelperClassName =
            "org/chromium/chrome/browser/tab_group_sync/StartupHelper";
    static String sBraveStartupHelperClassName =
            "org/chromium/chrome/browser/tab_group_sync/BraveStartupHelper";

    public BraveStartupHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sStartupHelperClassName, sBraveStartupHelperClassName);
        changeMethodOwner(
                sStartupHelperClassName,
                "handleUnsavedLocalTabGroups",
                sBraveStartupHelperClassName);
    }
}
