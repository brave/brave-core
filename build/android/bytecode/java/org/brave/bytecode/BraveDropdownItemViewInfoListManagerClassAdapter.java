/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDropdownItemViewInfoListManagerClassAdapter extends BraveClassVisitor {
    static String sDropdownItemViewInfoListManager =
            "org/chromium/chrome/browser/omnibox/suggestions/DropdownItemViewInfoListManager";

    static String sBraveDropdownItemViewInfoListManager =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveDropdownItemViewInfoListManager";

    public BraveDropdownItemViewInfoListManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sDropdownItemViewInfoListManager, sBraveDropdownItemViewInfoListManager);

        deleteMethod(sBraveDropdownItemViewInfoListManager, "removeSuggestionsForGroup");
        makePublicMethod(sDropdownItemViewInfoListManager, "removeSuggestionsForGroup");
    }
}
