/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCachedFeatureParamClassAdapter extends BraveClassVisitor {
    static String sCachedFeatureParamClassName =
            "org/chromium/components/cached_flags/CachedFeatureParam";
    static String sBraveCachedFeatureParamClassName =
            "org/chromium/components/cached_flags/BraveCachedFeatureParam";

    public BraveCachedFeatureParamClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sCachedFeatureParamClassName, "getAllInstances", sBraveCachedFeatureParamClassName);
    }
}
