/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutocompleteMediatorClassAdapter extends BraveClassVisitor {
    static String sAutocompleteMediator =
            "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator";
    static String sBraveAutocompleteMediator =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediator";

    public BraveAutocompleteMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sAutocompleteMediator, sBraveAutocompleteMediator);

        deleteField(sBraveAutocompleteMediator, "mNativeInitialized");
        makeProtectedField(sAutocompleteMediator, "mNativeInitialized");

        deleteField(sBraveAutocompleteMediator, "mDropdownViewInfoListManager");
        makeProtectedField(sAutocompleteMediator, "mDropdownViewInfoListManager");

        deleteField(sBraveAutocompleteMediator, "mDropdownViewInfoListBuilder");
        makeProtectedField(sAutocompleteMediator, "mDropdownViewInfoListBuilder");
    }
}
