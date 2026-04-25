/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveChromeContextMenuPopulatorAdapter extends BraveClassVisitor {
    static String sController =
            "org/chromium/chrome/browser/contextmenu/ChromeContextMenuPopulator";
    static String sBraveController =
            "org/chromium/chrome/browser/contextmenu/BraveChromeContextMenuPopulator";

    public BraveChromeContextMenuPopulatorAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(sController, sBraveController);

        deleteMethod(sBraveController, "getProfile");
        makePublicMethod(sController, "getProfile");

        deleteField(sBraveController, "mItemDelegate");
        makeProtectedField(sController, "mItemDelegate");

        deleteField(sBraveController, "mParams");
        makeProtectedField(sController, "mParams");
    }
}
