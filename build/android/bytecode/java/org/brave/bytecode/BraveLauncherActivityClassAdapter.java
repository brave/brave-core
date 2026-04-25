/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLauncherActivityClassAdapter extends BraveClassVisitor {
    static String sChromeLauncherActivityClassName =
            "org/chromium/chrome/browser/document/ChromeLauncherActivity";
    static String sBraveLauncherActivityClassName =
            "org/chromium/chrome/browser/document/BraveLauncherActivity";

    public BraveLauncherActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sChromeLauncherActivityClassName, sBraveLauncherActivityClassName);
    }
}
