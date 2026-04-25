/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveHomepageManagerClassAdapter extends BraveClassVisitor {
    static String sHomepageManagerClassName =
            "org/chromium/chrome/browser/homepage/HomepageManager";
    static String sBraveHomepageManagerClassName =
            "org/chromium/chrome/browser/homepage/BraveHomepageManager";

    public BraveHomepageManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sHomepageManagerClassName, sBraveHomepageManagerClassName);
    }
}
