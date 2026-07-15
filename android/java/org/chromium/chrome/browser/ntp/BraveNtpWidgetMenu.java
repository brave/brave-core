/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.native_page.BraveContextMenuManager;
import org.chromium.chrome.browser.native_page.BraveNtpDelegate;
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.native_page.NativePageNavigationDelegate;
import org.chromium.chrome.browser.preloading.AndroidPrerenderManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.LoadUrlParams;

/**
 * Shows the NTP top-sites widget-control options (show frequently visited / show shortcuts / hide
 * widget) on long-press of the "Add site" (+) button.
 *
 * <p>Reuses {@link BraveContextMenuManager} — the same icon-styled menu shown on tile long-press —
 * instead of a separate plain {@link android.widget.PopupMenu} layout, so the two menus can't drift
 * out of visual sync. {@link WidgetOnlyDelegate} reports only the widget-control items as
 * supported, so the standard Chromium items and "Add site" (tapping the button already adds a site)
 * never show — leaving just a divider and the three widget-control items.
 */
@NullMarked
public class BraveNtpWidgetMenu {
    /** Shows the widget-control menu anchored to {@code anchorView}. */
    public static void show(View anchorView) {
        BraveContextMenuManager manager =
                new BraveContextMenuManager(
                        new NoOpNavigationDelegate(), enabled -> {}, () -> {}, "NtpAddSiteButton");
        manager.showListContextMenu(anchorView, new WidgetOnlyDelegate());
    }

    private static class WidgetOnlyDelegate extends ContextMenuManager.EmptyDelegate
            implements BraveNtpDelegate {
        @Override
        public boolean isBraveItemSupported(int menuItemId) {
            return menuItemId == BRAVE_SHOW_FREQUENT
                    || menuItemId == BRAVE_SHOW_SHORTCUTS
                    || menuItemId == BRAVE_HIDE_WIDGET;
        }

        @Override
        public void braveAddSite() {}

        @Override
        public void braveShowFrequent() {
            NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
        }

        @Override
        public void braveShowShortcuts() {
            NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);
        }

        @Override
        public void braveHideWidget() {
            NtpUtil.setDisplayTopSites(false);
        }

        @Override
        public int getBraveTopSitesDisplayMode() {
            return NtpUtil.getTopSitesDisplayMode();
        }

        @Override
        public int getSelectedModeEndIconRes() {
            return R.drawable.ic_check_circle_filled;
        }
    }

    private static class NoOpNavigationDelegate implements NativePageNavigationDelegate {
        @Override
        public boolean isOpenInIncognitoEnabled() {
            return false;
        }

        @Override
        public boolean isOpenInOtherWindowEnabled() {
            return false;
        }

        @Override
        public @Nullable Tab openUrl(int windowOpenDisposition, LoadUrlParams loadUrlParams) {
            return null;
        }

        @Override
        public @Nullable Tab openUrlInGroup(
                int windowOpenDisposition, LoadUrlParams loadUrlParams) {
            return null;
        }

        @Override
        public void initAndroidPrerenderManager(AndroidPrerenderManager androidPrerenderManager) {}
    }
}
