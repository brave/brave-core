/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveVariationsSeedFetcherClassAdapter extends BraveClassVisitor {
    static String sVariationsSeedFetcherClassName =
            "org/chromium/components/variations/firstrun/VariationsSeedFetcher";
    static String sBraveVariationsSeedFetcherClassName =
            "org/chromium/components/variations/firstrun/BraveVariationsSeedFetcher";

    public BraveVariationsSeedFetcherClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sVariationsSeedFetcherClassName, "get", sBraveVariationsSeedFetcherClassName);
    }
}
