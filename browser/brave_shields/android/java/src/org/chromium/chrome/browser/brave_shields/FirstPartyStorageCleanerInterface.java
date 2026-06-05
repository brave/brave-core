/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_shields;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;

@NullMarked
public interface FirstPartyStorageCleanerInterface {
    void shredSiteData();

    boolean isShredButtonVisible();

    void setShredButtonVisibilityObserver(IncognitoReauthManager.IncognitoReauthCallback callback);

    void removeShredButtonVisibilityObserver(
            IncognitoReauthManager.IncognitoReauthCallback callback);
}
