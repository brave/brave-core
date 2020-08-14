/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCommandLineInitUtilClassAdapter extends BraveClassVisitor {
    static String sCommandLineClassName =
        "org/chromium/base/CommandLineInitUtil";

    static String sBraveCommandLineClassName =
        "org/chromium/base/BraveCommandLineInitUtil";

    static String sMethodReadFileAsUtf8 = "initCommandLine";

    public BraveCommandLineInitUtilClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(sCommandLineClassName, sMethodReadFileAsUtf8,
                          sBraveCommandLineClassName);
    }
}
