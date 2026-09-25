/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.webauthn;

import android.content.Context;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Brave override for {@link GmsCoreUtils}.
 *
 * <p>Upstream treats Google Play Services as a hard requirement for WebAuthn on Android: every
 * request is rejected with {@code NOT_IMPLEMENTED} unless the installed GMS Core is new enough for
 * the FIDO2 API. Android 14+ ships a platform Credential Manager that Chromium already knows how to
 * drive, so on devices without Play Services (GrapheneOS, LineageOS, ...) that requirement is the
 * only thing standing between the user and their third-party passkey provider.
 *
 * <p>{@code isWebauthnSupported} is redirected here by {@code BraveGmsCoreUtilsClassAdapter} and
 * additionally reports support when the platform Credential Manager can stand in for GMS.
 */
@NullMarked
public class BraveGmsCoreUtils {
    private static @Nullable Boolean sPlayServicesAvailable;

    /** Mirrors {@link GmsCoreUtils#isWebauthnSupported()} with the platform CredMan fallback. */
    public static boolean isWebauthnSupported() {
        return GmsCoreUtils.isWebauthnSupported() || shouldPreferPlatformCredMan();
    }

    /**
     * Returns whether WebAuthn requests should be served exclusively by the platform Credential
     * Manager: the device has one (Android 14+) and Play Services cannot provide the FIDO2 API.
     *
     * <p>When Play Services is available this returns false so behaviour is unchanged from
     * upstream; Google Password Manager keeps working exactly as before.
     */
    public static boolean shouldPreferPlatformCredMan() {
        return isPlatformCredManAvailable() && !arePlayServicesAvailable();
    }

    private static boolean isPlatformCredManAvailable() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.UPSIDE_DOWN_CAKE) return false;
        return ContextUtils.getApplicationContext().getSystemService(Context.CREDENTIAL_SERVICE)
                != null;
    }

    private static boolean arePlayServicesAvailable() {
        if (sPlayServicesAvailable == null) {
            boolean available;
            try {
                available = Fido2ApiCallHelper.getInstance().arePlayServicesAvailable();
            } catch (Exception e) {
                // Same defensive handling as Fido2CredentialRequest: any failure to talk to GMS
                // means it cannot serve FIDO2 requests.
                available = false;
            }
            sPlayServicesAvailable = available;
        }
        return sPlayServicesAvailable;
    }
}
