/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveWarmupManagerClassAdapter extends BraveClassVisitor {
    static String sChromeWarmupManagerClassName = "org/chromium/chrome/browser/WarmupManager";

    static String sBraveWarmupManagerClassName = "org/chromium/chrome/browser/BraveWarmupManager";

    public BraveWarmupManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sChromeWarmupManagerClassName, sBraveWarmupManagerClassName);
        changeMethodOwner(sChromeWarmupManagerClassName, "initializeViewHierarchy",
                sBraveWarmupManagerClassName);

        deleteMethod(sChromeWarmupManagerClassName, "initializeViewHierarchy");
    }
}
