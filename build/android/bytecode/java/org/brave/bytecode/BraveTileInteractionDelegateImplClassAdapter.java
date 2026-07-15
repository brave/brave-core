/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTileInteractionDelegateImplClassAdapter extends BraveClassVisitor {
    static String sTileInteractionDelegateImplClassName =
            "org/chromium/chrome/browser/suggestions/tile/TileInteractionDelegateImpl";
    static String sBraveTileInteractionDelegateImplClassName =
            "org/chromium/chrome/browser/suggestions/tile/BraveTileInteractionDelegateImpl";

    public BraveTileInteractionDelegateImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sTileInteractionDelegateImplClassName, sBraveTileInteractionDelegateImplClassName);
    }
}
