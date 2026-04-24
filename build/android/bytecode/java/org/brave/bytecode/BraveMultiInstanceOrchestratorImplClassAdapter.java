/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMultiInstanceOrchestratorImplClassAdapter extends BraveClassVisitor {
    static String sMultiInstanceOrchestratorImplClassName =
            "org/chromium/chrome/browser/multiwindow/MultiInstanceOrchestratorImpl";

    static String sBraveMultiInstanceOrchestratorImplClassName =
            "org/chromium/chrome/browser/multiwindow/BraveMultiInstanceOrchestratorImpl";

    public BraveMultiInstanceOrchestratorImplClassAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(
                sMultiInstanceOrchestratorImplClassName,
                sBraveMultiInstanceOrchestratorImplClassName);
    }
}
