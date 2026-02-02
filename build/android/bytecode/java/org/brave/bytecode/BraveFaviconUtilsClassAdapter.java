/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveFaviconUtilsClassAdapter extends BraveClassVisitor {
    static String sFaviconUtilsClassName = "org/chromium/chrome/browser/ui/favicon/FaviconUtils";
    static String sBraveFaviconUtilsClassName =
            "org/chromium/chrome/browser/ui/favicon/BraveFaviconUtils";

    public BraveFaviconUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sFaviconUtilsClassName, "getIconDrawableWithFilter", sBraveFaviconUtilsClassName);
    }
}
