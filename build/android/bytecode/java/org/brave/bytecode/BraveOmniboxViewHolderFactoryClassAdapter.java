/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveOmniboxViewHolderFactoryClassAdapter extends BraveClassVisitor {
    static String sOmniboxViewHolderFactory =
            "org/chromium/chrome/browser/omnibox/suggestions/OmniboxViewHolderFactory";

    static String sBraveOmniboxViewHolderFactory =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveOmniboxViewHolderFactory";

    static String sBraveOmniboxViewHolderFactoryDummySuper =
            "org/chromium/chrome/browser/omnibox/suggestions/BraveOmniboxViewHolderFactoryDummySuper"; // presubmit: ignore-long-line

    public BraveOmniboxViewHolderFactoryClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sBraveOmniboxViewHolderFactory, sOmniboxViewHolderFactory);

        redirectConstructor(sOmniboxViewHolderFactory, sBraveOmniboxViewHolderFactory);

        // registerType() is private in OmniboxViewHolderFactory; make it callable from
        // BraveOmniboxViewHolderFactory by bumping it to public and redirecting the DummySuper
        // stub calls to the real method.
        makePublicMethod(sOmniboxViewHolderFactory, "registerType");
        changeMethodOwner(
                sBraveOmniboxViewHolderFactoryDummySuper,
                "registerType",
                sOmniboxViewHolderFactory);
    }
}
