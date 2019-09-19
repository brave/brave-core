/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab;

import android.content.SharedPreferences;
import android.support.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.chrome.browser.preferences.website.DesktopModePreferences;

public class DesktopModeTabHelper extends TabWebContentsUserData {
    private static final Class<DesktopModeTabHelper> USER_DATA_KEY = DesktopModeTabHelper.class;

    private final boolean mDesktopModeEnabled;
    private DesktopModeWebContentsObsever mWebContentsObserver;

    private class DesktopModeWebContentsObsever extends WebContentsObserver {
        private final DesktopModeTabHelper mHelper;

        public DesktopModeWebContentsObsever(WebContents webContents, DesktopModeTabHelper helper) {
            super(webContents);
            mHelper = helper;
        }

        @Override
        public void navigationEntryCommitted() {
            updateDesktopMode();
        }

        private void updateDesktopMode() {
            mWebContents.get().getNavigationController().setUseDesktopUserAgent(
                mHelper.mDesktopModeEnabled, false);
        }
    }

    static DesktopModeTabHelper from(Tab tab) {
        DesktopModeTabHelper helper = get(tab);
        if (helper == null) {
            helper = tab.getUserDataHost().setUserData(USER_DATA_KEY, new DesktopModeTabHelper(tab));
        }
        return helper;
    }

    @Nullable
    public static DesktopModeTabHelper get(Tab tab) {
        return tab != null ? tab.getUserDataHost().getUserData(USER_DATA_KEY) : null;
    }

    private DesktopModeTabHelper(Tab tab) {
        super(tab);
        // This initial setting will be used during the life time of this tab.
        mDesktopModeEnabled = ContextUtils.getAppSharedPreferences().getBoolean(
            DesktopModePreferences.DESKTOP_MODE_KEY, false);
    }

    @Override
    public void initWebContents(WebContents webContents) {
        assert mWebContentsObserver == null;
        mWebContentsObserver = new DesktopModeWebContentsObsever(webContents, this);
    }

    @Override
    public void cleanupWebContents(WebContents webContents) {
        if (mWebContentsObserver == null)
            return;

        mWebContentsObserver.destroy();
        mWebContentsObserver = null;
    }

    public void ignoreDesktopMode() {
        if (mWebContentsObserver == null)
            return;

        // Don't override user agent anymore by initial desktop mode setting after
        // user explicitly toggle desktop view mode.
        mWebContentsObserver.destroy();
        mWebContentsObserver = null;
    }
}
