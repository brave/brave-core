/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCustomizationProviderDelegateImplClassAdapter extends BraveClassVisitor {
    static String sCustomizationProviderDelegateImplClassName =
            "org/chromium/chrome/browser/partnercustomizations/CustomizationProviderDelegateUpstreamImpl";
    static String sBraveCustomizationProviderDelegateImplClassName =
            "org/chromium/chrome/browser/partnercustomizations/BraveCustomizationProviderDelegateImpl";

    public BraveCustomizationProviderDelegateImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sCustomizationProviderDelegateImplClassName,
                sBraveCustomizationProviderDelegateImplClassName);
    }
}
