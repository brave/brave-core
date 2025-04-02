/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIntentClassAdapter extends BraveClassVisitor {
    static String sIntentClassName = "android/content/Intent";

    static String sBraveIntentClassName = "org/chromium/base/BraveIntent";

    public BraveIntentClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sIntentClassName, sBraveIntentClassName);
    }
}
