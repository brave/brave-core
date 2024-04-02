/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabUiThemeUtilsClassAdapter extends BraveClassVisitor {
    static String sTabUiThemeUtilsClassName = "org/chromium/chrome/browser/tab_ui/TabUiThemeUtils";
    static String sBraveTabUiThemeUtilsClassName =
            "org/chromium/chrome/browser/tab_ui/BraveTabUiThemeUtils";

    public BraveTabUiThemeUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sTabUiThemeUtilsClassName, "getTitleTextColor", sBraveTabUiThemeUtilsClassName);

        changeMethodOwner(
                sTabUiThemeUtilsClassName,
                "getCardViewBackgroundColor",
                sBraveTabUiThemeUtilsClassName);
    }
}
