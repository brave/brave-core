/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCachedFlagUtilsClassAdapter extends BraveClassVisitor {
    static String sCachedFlagUtilsClassName =
            "org/chromium/components/cached_flags/CachedFlagUtils";
    static String sBraveCachedFlagUtilsClassName =
            "org/chromium/components/cached_flags/BraveCachedFlagUtils";

    public BraveCachedFlagUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sCachedFlagUtilsClassName, "cacheNativeFlags", sBraveCachedFlagUtilsClassName);
        changeMethodOwner(
                sCachedFlagUtilsClassName, "cacheFeatureParams", sBraveCachedFlagUtilsClassName);
        changeMethodOwner(
                sCachedFlagUtilsClassName, "setFullListOfFlags", sBraveCachedFlagUtilsClassName);
        changeMethodOwner(
                sCachedFlagUtilsClassName,
                "setFullListOfFeatureParams",
                sBraveCachedFlagUtilsClassName);
    }
}
