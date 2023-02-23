/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabHelpersClassAdapter extends BraveClassVisitor {
    static String sTabHelpersClassName = "org/chromium/chrome/browser/tab/TabHelpers";
    static String sBraveTabHelpersClassName = "org/chromium/chrome/browser/tab/BraveTabHelpers";

    public BraveTabHelpersClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sTabHelpersClassName, "initTabHelpers", sBraveTabHelpersClassName);
    }
}
