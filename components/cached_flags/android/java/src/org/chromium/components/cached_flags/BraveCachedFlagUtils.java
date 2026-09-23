/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.cached_flags;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.ArrayList;
import java.util.List;

/**
 * Brave's extension of CachedFlagUtils to include Brave-specific cached flags and feature params.
 */
@NullMarked
public class BraveCachedFlagUtils extends CachedFlagUtils {
    private static @Nullable List<CachedFlag> sBraveFlags;
    private static @Nullable List<CachedFeatureParam<?>> sBraveParams;

    /** Set Brave-specific cached flags. */
    public static void setBraveFlags(List<CachedFlag> braveFlags) {
        sBraveFlags = braveFlags;
    }

    /** Set Brave-specific cached feature parameters. */
    public static void setBraveParams(List<CachedFeatureParam<?>> braveParams) {
        sBraveParams = braveParams;
    }

    /** Caches flags that must take effect on startup but are set via native code. */
    public static void cacheNativeFlags(List<List<CachedFlag>> listsOfFeaturesToCache) {
        List<List<CachedFlag>> allLists = new ArrayList<>(listsOfFeaturesToCache);
        if (sBraveFlags != null && !sBraveFlags.isEmpty()) {
            allLists.add(sBraveFlags);
        }
        CachedFlagUtils.cacheNativeFlags(allLists);
    }

    /** Caches feature params that must take effect on startup but are set via native code. */
    public static void cacheFeatureParams(List<List<CachedFeatureParam<?>>> listsOfParameters) {
        List<List<CachedFeatureParam<?>>> allLists = new ArrayList<>(listsOfParameters);
        if (sBraveParams != null && !sBraveParams.isEmpty()) {
            allLists.add(sBraveParams);
        }
        CachedFlagUtils.cacheFeatureParams(allLists);
    }

    /** Store a reference to the full list of CachedFlags for future use. */
    public static void setFullListOfFlags(List<List<CachedFlag>> listsOfFlags) {
        List<List<CachedFlag>> allLists = new ArrayList<>(listsOfFlags);
        if (sBraveFlags != null && !sBraveFlags.isEmpty()) {
            allLists.add(sBraveFlags);
        }
        CachedFlagUtils.setFullListOfFlags(allLists);
    }

    /** Store a reference to the full list of CachedFeatureParam for future use. */
    public static void setFullListOfFeatureParams(
            List<List<CachedFeatureParam<?>>> listsOfParameters) {
        List<List<CachedFeatureParam<?>>> allLists = new ArrayList<>(listsOfParameters);
        if (sBraveParams != null && !sBraveParams.isEmpty()) {
            allLists.add(sBraveParams);
        }
        CachedFlagUtils.setFullListOfFeatureParams(allLists);
    }
}
