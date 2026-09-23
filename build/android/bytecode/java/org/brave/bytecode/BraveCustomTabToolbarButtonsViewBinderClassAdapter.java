/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCustomTabToolbarButtonsViewBinderClassAdapter extends BraveClassVisitor {
    static String sViewBinderClassName =
            "org/chromium/chrome/browser/customtabs/features/toolbar/CustomTabToolbarButtonsViewBinder"; // presubmit: ignore-long-line
    static String sBraveViewBinderClassName =
            "org/chromium/chrome/browser/customtabs/features/toolbar/BraveCustomTabToolbarButtonsViewBinder"; // presubmit: ignore-long-line

    public BraveCustomTabToolbarButtonsViewBinderClassAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(sViewBinderClassName, sBraveViewBinderClassName);
    }
}
