/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTileViewClassAdapter extends BraveClassVisitor {
    static String sSuggestionsTileViewClassName =
            "org/chromium/chrome/browser/suggestions/tile/SuggestionsTileView";

    static String sBraveSuggestionsTileViewBaseClassName =
            "org/chromium/chrome/browser/suggestions/tile/BraveTileView";

    public BraveTileViewClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sSuggestionsTileViewClassName, sBraveSuggestionsTileViewBaseClassName);
    }
}
