/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.webauthn;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.ServiceImpl;
import org.chromium.components.webauthn.BraveGmsCoreUtils;
import org.chromium.components.webauthn.cred_man.CredManUiRecommender;

/**
 * Brave's {@link CredManUiRecommender}.
 *
 * <p>Chrome supplies this interface from Google-internal code; public builds have no
 * implementation, so {@code CredManSupportProvider} falls back to {@code PARALLEL_WITH_FIDO_2} and
 * routes requests through the Play Services FIDO2 API. Returning true here selects {@code
 * FULL_UNLESS_INAPPLICABLE}, i.e. the platform Credential Manager handles the request end to end.
 *
 * <p>Brave only recommends that when Play Services cannot serve FIDO2 requests. Devices with Play
 * Services keep upstream behaviour unchanged.
 */
@NullMarked
@ServiceImpl(CredManUiRecommender.class)
public class BraveCredManUiRecommender implements CredManUiRecommender {
    @Override
    public boolean recommendsCustomUi() {
        return BraveGmsCoreUtils.shouldPreferPlatformCredMan();
    }
}
