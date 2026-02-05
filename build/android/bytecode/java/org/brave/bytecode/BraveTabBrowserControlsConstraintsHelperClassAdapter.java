/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabBrowserControlsConstraintsHelperClassAdapter extends BraveClassVisitor {
    private static final String sTabBrowserControlsConstraintsHelper =
            "org/chromium/chrome/browser/tab/TabBrowserControlsConstraintsHelper";
    private static final String sBraveTabBrowserControlsConstraintsHelper =
            "org/chromium/chrome/browser/tab/BraveTabBrowserControlsConstraintsHelper";

    public BraveTabBrowserControlsConstraintsHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sTabBrowserControlsConstraintsHelper,
                "getObservableConstraints",
                sBraveTabBrowserControlsConstraintsHelper);
    }
}
