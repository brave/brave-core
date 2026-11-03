/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.dom_distiller;

import androidx.annotation.VisibleForTesting;

import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.user_prefs.UserPrefs;

import java.util.function.Supplier;

public class BraveReaderModeManager extends ReaderModeManager {
    // To be removed in bytecode, parent variable will be used instead.
    private Tab mTab;

    BraveReaderModeManager(Tab tab, Supplier<MessageDispatcher> messageDispatcherSupplier) {
        super(tab, messageDispatcherSupplier);
    }

    @VisibleForTesting
    @Override
    boolean tryShowingPrompt(boolean resetRestorePrompt) {
        if (!isReaderForAccessibilityEnabled(mTab)) return false;

        return super.tryShowingPrompt(resetRestorePrompt);
    }

    /**
     * Calls to {@link ReaderModeManager#shouldUseReaderModeMessages} are redirected here via
     * bytecode. Regular tabs get the prompt too, rather than only custom tabs.
     */
    public static boolean shouldUseReaderModeMessages(@Nullable Tab tab) {
        if (ReaderModeManager.shouldUseReaderModeMessages(tab)) return true;
        return tab != null && !tab.isIncognito() && isReaderForAccessibilityEnabled(tab);
    }

    private static boolean isReaderForAccessibilityEnabled(@Nullable Tab tab) {
        if (tab == null || tab.getWebContents() == null) return false;

        Profile profile = Profile.fromWebContents(tab.getWebContents());
        return profile != null && UserPrefs.get(profile).getBoolean(Pref.READER_FOR_ACCESSIBILITY);
    }
}
