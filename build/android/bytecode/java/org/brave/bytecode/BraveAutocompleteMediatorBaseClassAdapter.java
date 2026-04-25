/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutocompleteMediatorBaseClassAdapter extends BraveClassVisitor {
    static String sAutocompleteMediator =
            "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteMediator";
    static String sBraveAutocompleteMediatorBase =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediatorBase";

    public BraveAutocompleteMediatorBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sAutocompleteMediator, sBraveAutocompleteMediatorBase);

        makeProtectedField(sAutocompleteMediator, "mContext");
        makeProtectedField(sAutocompleteMediator, "mDataProvider");
        changeMethodOwner(
                sAutocompleteMediator, "loadUrlForOmniboxMatch", sBraveAutocompleteMediatorBase);
    }
}
