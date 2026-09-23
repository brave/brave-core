/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveEditUrlSuggestionProcessorBaseClassAdapter extends BraveClassVisitor {
    static String sEditUrlSuggestionProcessor =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor";
    static String sBraveEditUrlSuggestionProcessorBase =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessorBase";

    public BraveEditUrlSuggestionProcessorBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sEditUrlSuggestionProcessor, sBraveEditUrlSuggestionProcessorBase);

        changeMethodOwner(
                sEditUrlSuggestionProcessor, "onCopyLink", sBraveEditUrlSuggestionProcessorBase);
    }
}
