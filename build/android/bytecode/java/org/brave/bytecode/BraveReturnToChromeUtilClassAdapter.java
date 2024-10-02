/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveReturnToChromeUtilClassAdapter extends BraveClassVisitor {
    static String sReturnToChromeUtilClassName =
            "org/chromium/chrome/browser/tasks/ReturnToChromeUtil";
    static String sBraveReturnToChromeUtilClassName =
            "org/chromium/chrome/browser/tasks/BraveReturnToChromeUtil";

    public BraveReturnToChromeUtilClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sReturnToChromeUtilClassName,
                "shouldShowTabSwitcher",
                sBraveReturnToChromeUtilClassName);

        changeMethodOwner(
                sReturnToChromeUtilClassName,
                "shouldShowNtpAsHomeSurfaceAtStartup",
                sBraveReturnToChromeUtilClassName);
    }
}
