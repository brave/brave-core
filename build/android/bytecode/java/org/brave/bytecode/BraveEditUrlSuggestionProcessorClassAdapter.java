/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveEditUrlSuggestionProcessorClassAdapter extends BraveClassVisitor {
    static String sEditUrlSuggestionProcessor =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor";
    static String sBraveEditUrlSuggestionProcessor =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessor";

    public BraveEditUrlSuggestionProcessorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sEditUrlSuggestionProcessor, sBraveEditUrlSuggestionProcessor);

        deleteField(sBraveEditUrlSuggestionProcessor, "mHasClearedOmniboxForFocus");
        makeProtectedField(sEditUrlSuggestionProcessor, "mHasClearedOmniboxForFocus");
    }
}
