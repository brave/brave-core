/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMostVisitedTilesLayoutBaseClassAdapter extends BraveClassVisitor {
    static String sMostVisitedTilesLayoutClassName =
            "org/chromium/chrome/browser/suggestions/tile/MostVisitedTilesLayout";
    static String sBraveMostVisitedTilesLayoutBaseClassName =
            "org/chromium/chrome/browser/suggestions/tile/BraveMostVisitedTilesLayoutBase";

    public BraveMostVisitedTilesLayoutBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(
                sMostVisitedTilesLayoutClassName, sBraveMostVisitedTilesLayoutBaseClassName);

        changeSuperName(
                "org/chromium/chrome/browser/suggestions/tile/TilesLinearLayout",
                "android/widget/GridLayout");

        deleteMethod(sBraveMostVisitedTilesLayoutBaseClassName, "getColumnCount");
        deleteMethod(sBraveMostVisitedTilesLayoutBaseClassName, "setColumnCount");
        deleteMethod(sBraveMostVisitedTilesLayoutBaseClassName, "getRowCount");
        deleteMethod(sBraveMostVisitedTilesLayoutBaseClassName, "setRowCount");
    }
}
