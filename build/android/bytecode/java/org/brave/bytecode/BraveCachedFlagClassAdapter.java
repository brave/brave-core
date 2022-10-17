/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCachedFlagClassAdapter extends BraveClassVisitor {
    static String sCachedFlagClassName = "org/chromium/chrome/browser/flags/CachedFlag";
    static String sBraveCachedFlagClassName = "org/chromium/chrome/browser/flags/BraveCachedFlag";

    public BraveCachedFlagClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sCachedFlagClassName, sBraveCachedFlagClassName);
    }
}
