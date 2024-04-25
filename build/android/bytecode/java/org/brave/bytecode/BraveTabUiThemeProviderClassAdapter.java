/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabUiThemeProviderClassAdapter extends BraveClassVisitor {
    static String sTabUiThemeProviderClassName =
            "org/chromium/chrome/browser/tasks/tab_management/TabUiThemeProvider";
    static String sBraveTabUiThemeProviderClassName =
            "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiThemeProvider";

    public BraveTabUiThemeProviderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sTabUiThemeProviderClassName,
                "getActionButtonTintList",
                sBraveTabUiThemeProviderClassName);
    }
}
