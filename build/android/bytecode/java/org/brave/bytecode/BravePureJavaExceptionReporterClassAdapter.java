/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePureJavaExceptionReporterClassAdapter extends BraveClassVisitor {
    static String sChromePureJavaExceptionReporterClassName =
            "org/chromium/chrome/browser/crash/ChromePureJavaExceptionReporter";
    static String sBravePureJavaExceptionReporterClassName =
            "org/chromium/chrome/browser/crash/BravePureJavaExceptionReporter";

    public BravePureJavaExceptionReporterClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sChromePureJavaExceptionReporterClassName,
                sBravePureJavaExceptionReporterClassName);
    }
}
