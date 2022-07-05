/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.dom_distiller;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveReaderModeManager extends ReaderModeManager {
    // To be removed in bytecode, parent variable will be used instead.
    private Tab mTab;

    BraveReaderModeManager(Tab tab, Supplier<MessageDispatcher> messageDispatcherSupplier) {
        super(tab, messageDispatcherSupplier);
    }

    @VisibleForTesting
    @Override
    void tryShowingPrompt() {
        if (mTab == null || mTab.getWebContents() == null) return;

        Profile profile = Profile.fromWebContents(mTab.getWebContents());
        if (profile == null || !UserPrefs.get(profile).getBoolean(Pref.READER_FOR_ACCESSIBILITY))
            return;

        super.tryShowingPrompt();
    }
}
