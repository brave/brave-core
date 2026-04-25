/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

        deleteField(sBraveVariationsSeedFetcherClassName, "sLock");
        makeProtectedField(sVariationsSeedFetcherClassName, "sLock");
        deleteField(sBraveVariationsSeedFetcherClassName, "DEFAULT_VARIATIONS_SERVER_URL");
        makeProtectedField(sVariationsSeedFetcherClassName, "DEFAULT_VARIATIONS_SERVER_URL");
        deleteField(sBraveVariationsSeedFetcherClassName, "DEFAULT_FAST_VARIATIONS_SERVER_URL");
        makeProtectedField(sVariationsSeedFetcherClassName, "DEFAULT_FAST_VARIATIONS_SERVER_URL");
    }
}
