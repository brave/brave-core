/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveApplicationImplBaseClassAdapter extends BraveClassVisitor {
    static String sChromeApplicationImplClassName =
            "org/chromium/chrome/browser/ChromeApplicationImpl";

    static String sBraveApplicationImplBaseClassName =
            "org/chromium/chrome/browser/BraveApplicationImplBase";

    public BraveApplicationImplBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sChromeApplicationImplClassName, sBraveApplicationImplBaseClassName);
    }
}
