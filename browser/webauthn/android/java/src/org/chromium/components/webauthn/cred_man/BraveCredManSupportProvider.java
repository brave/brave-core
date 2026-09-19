/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.webauthn.cred_man;

import org.chromium.build.annotations.NullMarked;
import org.chromium.components.webauthn.BraveGmsCoreUtils;

/**
 * Brave override for {@link CredManSupportProvider}.
 *
 * <p>{@code CredManSupportProvider#getCredManSupport} marks CredMan as {@code DISABLED} whenever
 * the installed GMS Core is older than the version Chrome requires, before it ever asks the
 * embedder's {@code CredManUiRecommender}. Without Play Services the reported version is -1, so
 * the platform Credential Manager would never be consulted even though it is fully functional.
 *
 * <p>{@code hasOldGmsVersion} is redirected here by {@code BraveCredManSupportProviderClassAdapter}
 * and skips the GMS version gate when the platform Credential Manager should serve the request.
 */
@NullMarked
public class BraveCredManSupportProvider {
    /** Mirrors {@code CredManSupportProvider#hasOldGmsVersion()}. */
    public static boolean hasOldGmsVersion() {
        if (BraveGmsCoreUtils.shouldPreferPlatformCredMan()) return false;
        return BraveCredManSupportProviderDummySuper.hasOldGmsVersion();
    }
}
