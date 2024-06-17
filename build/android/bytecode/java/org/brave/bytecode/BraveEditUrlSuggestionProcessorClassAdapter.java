/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveEditUrlSuggestionProcessorClassAdapter extends BraveClassVisitor {
    static String sEditUrlSuggestionProcessorName =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/EditUrlSuggestionProcessor";
    static String sBraveEditUrlSuggestionProcessorName =
            "org/chromium/chrome/browser/omnibox/suggestions/editurl/BraveEditUrlSuggestionProcessor";

    public BraveEditUrlSuggestionProcessorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sEditUrlSuggestionProcessorName, sBraveEditUrlSuggestionProcessorName);

        makePublicMethod(sEditUrlSuggestionProcessorName, "onCopyLink");
        addMethodAnnotation(
                sBraveEditUrlSuggestionProcessorName, "onCopyLink", "Ljava/lang/Override;");
    }
}
