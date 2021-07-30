/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStatusMediatorClassAdapter extends BraveClassVisitor {
    static String sStatusMediatorClassName =
            "org/chromium/chrome/browser/omnibox/status/StatusMediator";
    static String sBraveStatusMediatorClassName =
            "org/chromium/chrome/browser/omnibox/status/BraveStatusMediator";

    public BraveStatusMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sStatusMediatorClassName, sBraveStatusMediatorClassName);

        deleteField(sBraveStatusMediatorClassName, "mUrlHasFocus");
        makeProtectedField(sStatusMediatorClassName, "mUrlHasFocus");
    }
}
