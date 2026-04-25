/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLocationBarCoordinatorClassAdapter extends BraveClassVisitor {
    static String sLocationBarCoordinator =
            "org/chromium/chrome/browser/omnibox/LocationBarCoordinator";
    static String sBraveLocationBarCoordinator =
            "org/chromium/chrome/browser/omnibox/BraveLocationBarCoordinator";

    public BraveLocationBarCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sLocationBarCoordinator, sBraveLocationBarCoordinator);

        deleteField(sBraveLocationBarCoordinator, "mLocationBarMediator");
        makeProtectedField(sLocationBarCoordinator, "mLocationBarMediator");

        deleteField(sBraveLocationBarCoordinator, "mUrlBar");
        makeProtectedField(sLocationBarCoordinator, "mUrlBar");
    }
}
