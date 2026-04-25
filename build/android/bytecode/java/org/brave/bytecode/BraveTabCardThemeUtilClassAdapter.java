/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabCardThemeUtilClassAdapter extends BraveClassVisitor {
    static String sTabCardThemeUtilClassName =
            "org/chromium/chrome/browser/tab_ui/TabCardThemeUtil";
    static String sBraveTabCardThemeUtilClassName =
            "org/chromium/chrome/browser/tab_ui/BraveTabCardThemeUtil";

    public BraveTabCardThemeUtilClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sTabCardThemeUtilClassName, "getTitleTextColor", sBraveTabCardThemeUtilClassName);

        changeMethodOwner(
                sTabCardThemeUtilClassName,
                "getCardViewBackgroundColor",
                sBraveTabCardThemeUtilClassName);

        changeMethodOwner(
                sTabCardThemeUtilClassName,
                "getActionButtonTintList",
                sBraveTabCardThemeUtilClassName);
    }
}
