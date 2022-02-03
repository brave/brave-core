/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCachedFeatureFlagsClassAdapter extends BraveClassVisitor {
    static String sCachedFeatureFlagsClassName =
            "org/chromium/chrome/browser/flags/CachedFeatureFlags";
    static String sBraveCachedFeatureFlagsClassName =
            "org/chromium/chrome/browser/flags/BraveCachedFeatureFlags";

    public BraveCachedFeatureFlagsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sCachedFeatureFlagsClassName, "getConsistentBooleanValue",
                sBraveCachedFeatureFlagsClassName);
    }
}
