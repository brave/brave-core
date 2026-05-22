/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveOmniboxSuggestionsDropdownAdapterClassAdapter extends BraveClassVisitor {
    static String sOmniboxSuggestionsDropdownAdapter =
            "org/chromium/chrome/browser/omnibox/suggestions/OmniboxSuggestionsDropdownAdapter";

    static String sBraveOmniboxSuggestionsDropdownAdapter =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveOmniboxSuggestionsDropdownAdapter";

    public BraveOmniboxSuggestionsDropdownAdapterClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sOmniboxSuggestionsDropdownAdapter, sBraveOmniboxSuggestionsDropdownAdapter);
    }
}
