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
    static String sDeferredContentViewStubClassName =
            "org/chromium/chrome/browser/tab/TabImpl$DeferredContentViewStub";

    public BraveContentViewClassAdapter(ClassVisitor visitor) {
        super(visitor);

        // The constructor redirect below also rewrites this subclass' `super(...)` call, so
        // its superclass has to be changed to match or it fails dex verification.
        changeSuperName(sDeferredContentViewStubClassName, sBraveContentViewClassName);

        redirectConstructor(sContentViewClassName, sBraveContentViewClassName);
    }
}
