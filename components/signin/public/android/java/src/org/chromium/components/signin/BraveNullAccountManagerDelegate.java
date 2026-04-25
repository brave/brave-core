/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.signin;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public class BraveNullAccountManagerDelegate extends NullAccountManagerDelegate {
    public BraveNullAccountManagerDelegate() {
        super();
    }

    @Override
    public void invalidateAccessToken(String authToken) throws AuthException {
        // No-op implementation. Token invalidation is not needed because:
        // 1. This delegate doesn't fetch tokens via getAccessToken(), so no tokens
        //    should exist from this delegate's perspective.
        // 2. However, tokens can still be passed here from other sources.
        // 3. Chromium already removes tokens from its internal cache before calling
        //    this method, so the token is already invalidated from Chromium's perspective.
        // 4. This method would normally call GoogleAuthUtil.clearToken() to invalidate
        //    the token in Android's AccountManager cache, but since we're not managing
        //    real Google accounts, there's nothing to clear at the system level.
    }
}
