/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMostVisitedTilesMediatorClassAdapter extends BraveClassVisitor {
    static String sMostVisitedTilesMediatorClassName =
            "org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesMediator";
    static String sBraveMostVisitedTilesMediatorClassName =
            "org/chromium/chrome/browser/suggestions/tile/BraveMostVisitedTilesMediator";

    public BraveMostVisitedTilesMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sMostVisitedTilesMediatorClassName, sBraveMostVisitedTilesMediatorClassName);

        makePublicMethod(sMostVisitedTilesMediatorClassName, "updateTilePlaceholderVisibility");
        addMethodAnnotation(sBraveMostVisitedTilesMediatorClassName,
                "updateTilePlaceholderVisibility", "Ljava/lang/Override;");

        deleteField(sBraveMostVisitedTilesMediatorClassName, "mTileGroup");
        makeProtectedField(sMostVisitedTilesMediatorClassName, "mTileGroup");
    }
}
