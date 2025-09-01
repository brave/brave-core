/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSearchBarCoordinatorClassAdapter extends BraveClassVisitor {
    static String sSearchBarCoordinatorClassName =
            "org/chromium/chrome/browser/download/home/search/SearchBarCoordinator";
    static String sBraveSearchBarCoordinatorClassName =
            "org/chromium/chrome/browser/download/home/search/BraveSearchBarCoordinator";

    public BraveSearchBarCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sSearchBarCoordinatorClassName, sBraveSearchBarCoordinatorClassName);
    }
}
