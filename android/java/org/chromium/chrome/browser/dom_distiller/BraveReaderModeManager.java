/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.dom_distiller;

import android.app.Activity;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabUtils;
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

        // If it is regular tab, we pretend to be a custom tab to show the prompt if applicable.
        spoofCustomTab(!mTab.isCustomTab() && !mTab.isIncognito());

        super.tryShowingPrompt();

        // There is no need to spoof custom tab after showing the prompt.
        spoofCustomTab(false);
    }

    /*
     * Whether we want to pretend to be a custom tab. Used here to avoid patch in the middle of `ReaderModeManager#tryShowingPrompt`.
     */
    void spoofCustomTab(boolean spoof) {
        Activity activity = TabUtils.getActivity(mTab);
        BraveActivity braveActivity =
                activity instanceof BraveActivity ? (BraveActivity) activity : null;
        if (braveActivity != null) {
            braveActivity.spoofCustomTab(spoof);
        }
    }
}
