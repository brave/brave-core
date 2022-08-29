/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLocationBarLayoutClassAdapter extends BraveClassVisitor {
    static String sBraveLocationBarLayout =
            "org/chromium/chrome/browser/omnibox/BraveLocationBarLayout";
    static String sLocationBarPhone = "org/chromium/chrome/browser/omnibox/LocationBarPhone";
    static String sLocationBarTablet = "org/chromium/chrome/browser/omnibox/LocationBarTablet";
    static String sSearchActivityLocationBarLayout =
            "org/chromium/chrome/browser/searchwidget/SearchActivityLocationBarLayout";

    public BraveLocationBarLayoutClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sLocationBarPhone, sBraveLocationBarLayout);
        changeSuperName(sLocationBarTablet, sBraveLocationBarLayout);
        changeSuperName(sSearchActivityLocationBarLayout, sBraveLocationBarLayout);
    }
}
