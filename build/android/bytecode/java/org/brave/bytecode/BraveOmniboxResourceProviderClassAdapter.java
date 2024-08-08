/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveOmniboxResourceProviderClassAdapter extends BraveClassVisitor {
    static String sOmniboxResourceProviderClassName =
            "org/chromium/chrome/browser/omnibox/styles/OmniboxResourceProvider";
    static String sBraveOmniboxResourceProviderClassName =
            "org/chromium/chrome/browser/omnibox/styles/BraveOmniboxResourceProvider";

    BraveOmniboxResourceProviderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sOmniboxResourceProviderClassName,
                "getToolbarSidePaddingForNtp",
                sBraveOmniboxResourceProviderClassName);
    }
}
