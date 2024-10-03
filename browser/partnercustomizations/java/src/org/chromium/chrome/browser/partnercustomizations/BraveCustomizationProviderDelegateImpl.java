/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.partnercustomizations;

/** Default ContextProviderPackageDelegateImpl implementation. */
public class BraveCustomizationProviderDelegateImpl
        extends CustomizationProviderDelegateUpstreamImpl {
    @Override
    public String getHomepage() {
        return null;
    }

    @Override
    public boolean isIncognitoModeDisabled() {
        return false;
    }

    @Override
    public boolean isBookmarksEditingDisabled() {
        return false;
    }
}