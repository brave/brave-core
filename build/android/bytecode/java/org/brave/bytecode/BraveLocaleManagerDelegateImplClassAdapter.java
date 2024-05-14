/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLocaleManagerDelegateImplClassAdapter extends BraveClassVisitor {
    static String sLocaleManagerDelegateImpl =
            "org/chromium/components/language/LocaleManagerDelegateImpl";
    static String sBraveLocaleManagerDelegateImpl =
            "org/chromium/components/language/BraveLocaleManagerDelegateImpl";

    public BraveLocaleManagerDelegateImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sLocaleManagerDelegateImpl, sBraveLocaleManagerDelegateImpl);
    }
}
