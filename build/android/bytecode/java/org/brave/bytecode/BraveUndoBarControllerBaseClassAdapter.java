/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveUndoBarControllerBaseClassAdapter extends BraveClassVisitor {
    static String sUndoBarController =
            "org/chromium/chrome/browser/undo_tab_close_snackbar/UndoBarController";
    static String sBraveUndoBarControllerBase =
            "org/chromium/chrome/browser/undo_tab_close_snackbar/BraveUndoBarControllerBase";

    public BraveUndoBarControllerBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sUndoBarController, sBraveUndoBarControllerBase);

        changeMethodOwner(sUndoBarController, "showUndoBar", sBraveUndoBarControllerBase);
    }
}
