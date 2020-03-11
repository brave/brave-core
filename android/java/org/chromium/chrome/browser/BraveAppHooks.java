/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.chrome.browser.help.BraveHelpAndFeedback;
import org.chromium.chrome.browser.help.HelpAndFeedback;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmark;
import org.chromium.chrome.browser.partnercustomizations.BravePartnerBrowserCustomizations;
import org.chromium.chrome.browser.partnercustomizations.PartnerBrowserCustomizations;

public class BraveAppHooks extends AppHooksImpl {
    @Override
    public HelpAndFeedback createHelpAndFeedback() {
        return new BraveHelpAndFeedback();
    }

    @Override
    public PartnerBookmark.BookmarkIterator getPartnerBookmarkIterator() {
        return null;
    }

    @Override
    public PartnerBrowserCustomizations.Provider getCustomizationProvider() {
        return new BravePartnerBrowserCustomizations.ProviderPackage();
    }
}
