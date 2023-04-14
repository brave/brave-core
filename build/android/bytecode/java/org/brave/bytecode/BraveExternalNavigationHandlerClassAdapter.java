/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveExternalNavigationHandlerClassAdapter extends BraveClassVisitor {
    static String sExternalNavigationHandlerClassName =
            "org/chromium/components/external_intents/ExternalNavigationHandler";
    static String sBraveBraveExternalNavigationHandlerClassName =
            "org/chromium/chrome/browser/externalnav/BraveExternalNavigationHandler";

    public BraveExternalNavigationHandlerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sExternalNavigationHandlerClassName, sBraveBraveExternalNavigationHandlerClassName);
    }
}
