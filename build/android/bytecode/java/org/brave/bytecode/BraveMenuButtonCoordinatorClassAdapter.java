/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMenuButtonCoordinatorClassAdapter extends BraveClassVisitor {
    static String sMenuButtonCoordinator =
            "org/chromium/chrome/browser/toolbar/menu_button/MenuButtonCoordinator";
    static String sBraveMenuButtonCoordinator =
            "org/chromium/chrome/browser/toolbar/menu_button/BraveMenuButtonCoordinator";

    public BraveMenuButtonCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sMenuButtonCoordinator, sBraveMenuButtonCoordinator);
    }
}
