/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveToolbarLayoutClassAdapter extends BraveClassVisitor {
    static String sCustomTabToolbarClassName =
            "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbar";
    static String sToolbarPhoneClassName = "org/chromium/chrome/browser/toolbar/top/ToolbarPhone";
    static String sToolbarTabletClassName = "org/chromium/chrome/browser/toolbar/top/ToolbarTablet";
    static String sBraveToolbarLayoutClassName =
            "org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl";

    public BraveToolbarLayoutClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sCustomTabToolbarClassName, sBraveToolbarLayoutClassName);

        changeSuperName(sToolbarPhoneClassName, sBraveToolbarLayoutClassName);

        changeSuperName(sToolbarTabletClassName, sBraveToolbarLayoutClassName);

        deleteMethod(sToolbarPhoneClassName, "onHomeButtonUpdate");
    }
}
