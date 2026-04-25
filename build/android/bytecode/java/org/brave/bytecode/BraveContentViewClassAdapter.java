/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveContentViewClassAdapter extends BraveClassVisitor {
    static String sContentViewClassName =
            "org/chromium/components/embedder_support/view/ContentView";
    static String sBraveContentViewClassName =
            "org/chromium/components/embedder_support/view/BraveContentView";

    public BraveContentViewClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sContentViewClassName, sBraveContentViewClassName);
    }
}
