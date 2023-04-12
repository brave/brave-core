/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutocompleteCoordinatorClassAdapter extends BraveClassVisitor {
    static String sAutocompleteCoordinator =
            "org/chromium/chrome/browser/omnibox/suggestions/AutocompleteCoordinator";

    static String sBraveAutocompleteCoordinator =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteCoordinator";

    public BraveAutocompleteCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sAutocompleteCoordinator, sBraveAutocompleteCoordinator);
        changeMethodOwner(
                sAutocompleteCoordinator, "createViewProvider", sBraveAutocompleteCoordinator);
    }
}
