/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClearBrowsingDataFragmentAdvancedAdapter extends BraveClassVisitor {
    static String sFragment =
            "org/chromium/chrome/browser/browsing_data/ClearBrowsingDataFragmentAdvanced";
    static String sBraveFragment =
            "org/chromium/chrome/browser/browsing_data/BraveClearBrowsingDataFragmentAdvanced";

    public BraveClearBrowsingDataFragmentAdvancedAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(sFragment, sBraveFragment);
    }
}
