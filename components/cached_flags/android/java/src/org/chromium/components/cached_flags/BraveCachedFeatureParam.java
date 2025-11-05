/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.cached_flags;

import org.chromium.build.annotations.CheckDiscard;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Class to override getAllInstances method.
 *
 * @param <T> The type parameter for the cached feature parameter.
 */
@NullMarked
public abstract class BraveCachedFeatureParam<T> {
    public static @Nullable Set<CachedFeatureParam<?>> sAllBraveInstances;

    /** Set Brave-specific cached feature parameters. */
    public static void setBraveParams(Collection<CachedFeatureParam<?>> braveParams) {
        if (sAllBraveInstances == null) {
            sAllBraveInstances = new HashSet<>();
        }
        sAllBraveInstances.addAll(braveParams);
    }

    @CheckDiscard("crbug.com/1067145")
    public static @Nullable Set<CachedFeatureParam<?>> getAllInstances() {
        Set<CachedFeatureParam<?>> allInstances = CachedFeatureParam.getAllInstances();
        if (allInstances == null || sAllBraveInstances == null) {
            return allInstances;
        }
        Set<CachedFeatureParam<?>> result = new HashSet<>(allInstances);
        // Remove all Brave instances from the result as it triggers an assert
        // in ChromeCachedFlags.tryToCatchMissingParameters,
        // but we handle everything in BraveCachedFlags.
        result.removeAll(sAllBraveInstances);
        return result;
    }
}
