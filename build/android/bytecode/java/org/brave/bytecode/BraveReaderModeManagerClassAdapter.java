/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveReaderModeManagerClassAdapter extends BraveClassVisitor {
    static String sReaderModeManagerClassName =
            "org/chromium/chrome/browser/dom_distiller/ReaderModeManager";
    static String sBraveReaderModeManagerClassName =
            "org/chromium/chrome/browser/dom_distiller/BraveReaderModeManager";

    public BraveReaderModeManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sReaderModeManagerClassName, sBraveReaderModeManagerClassName);

        deleteField(sBraveReaderModeManagerClassName, "mTab");
        makeProtectedField(sReaderModeManagerClassName, "mTab");
    }
}
