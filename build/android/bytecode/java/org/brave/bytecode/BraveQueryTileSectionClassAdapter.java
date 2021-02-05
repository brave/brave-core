/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveQueryTileSectionClassAdapter extends BraveClassVisitor {
    static String sQueryTileSectionClassName =
            "org/chromium/chrome/browser/query_tiles/QueryTileSection";
    static String sBraveQueryTileSectionClassName =
            "org/chromium/chrome/browser/query_tiles/BraveQueryTileSection";

    public BraveQueryTileSectionClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sQueryTileSectionClassName, "getMaxRowsForMostVisitedTiles",
                sBraveQueryTileSectionClassName);
    }
}
