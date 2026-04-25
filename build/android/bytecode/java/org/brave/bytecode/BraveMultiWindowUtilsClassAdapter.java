/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMultiWindowUtilsClassAdapter extends BraveClassVisitor {
    static String sMultiWindowUtilsClassName =
            "org/chromium/chrome/browser/multiwindow/MultiWindowUtils";
    static String sBraveMultiWindowUtilsClassName =
            "org/chromium/chrome/browser/multiwindow/BraveMultiWindowUtils";

    public BraveMultiWindowUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sMultiWindowUtilsClassName, sBraveMultiWindowUtilsClassName);
        changeMethodOwner(
                sMultiWindowUtilsClassName,
                "shouldShowManageWindowsMenu",
                sBraveMultiWindowUtilsClassName);
    }
}
