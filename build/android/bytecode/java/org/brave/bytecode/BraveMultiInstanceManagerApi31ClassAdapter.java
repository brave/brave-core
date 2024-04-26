/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMultiInstanceManagerApi31ClassAdapter extends BraveClassVisitor {
    static String sMultiInstanceManagerApi31 =
            "org/chromium/chrome/browser/multiwindow/MultiInstanceManagerApi31";
    static String sBraveMultiInstanceManagerApi31 =
            "org/chromium/chrome/browser/multiwindow/BraveMultiInstanceManagerApi31";

    public BraveMultiInstanceManagerApi31ClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sMultiInstanceManagerApi31, sBraveMultiInstanceManagerApi31);

        deleteField(sBraveMultiInstanceManagerApi31, "mInstanceId");
        makeProtectedField(sMultiInstanceManagerApi31, "mInstanceId");
    }
}
