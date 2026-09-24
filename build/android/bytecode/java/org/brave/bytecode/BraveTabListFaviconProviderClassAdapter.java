/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabListFaviconProviderClassAdapter extends BraveClassVisitor {
    private static final String sTabListFaviconProviderClassName =
            "org/chromium/chrome/browser/tab_ui/TabListFaviconProvider";
    private static final String sBraveTabListFaviconProviderClassName =
            "org/chromium/chrome/browser/ui/favicon/BraveTabListFaviconProvider";

    public BraveTabListFaviconProviderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sTabListFaviconProviderClassName, sBraveTabListFaviconProviderClassName);
    }
}
