/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDropdownItemViewInfoListBuilderClassAdapter extends BraveClassVisitor {
    static String sDropdownItemViewInfoListBuilder =
            "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListBuilder";

    static String sBraveDropdownItemViewInfoListBuilder =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveDropdownItemViewInfoListBuilder";

    public BraveDropdownItemViewInfoListBuilderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sDropdownItemViewInfoListBuilder, sBraveDropdownItemViewInfoListBuilder);
    }
}
