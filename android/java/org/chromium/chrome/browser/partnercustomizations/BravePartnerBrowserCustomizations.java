/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.partnercustomizations;

public class BravePartnerBrowserCustomizations {
    public static class ProviderPackage implements PartnerBrowserCustomizations.Provider {
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
}
