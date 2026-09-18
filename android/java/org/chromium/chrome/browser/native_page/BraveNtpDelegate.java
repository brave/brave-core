/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.native_page;

import org.chromium.build.annotations.NullMarked;

/**
 * Brave extension of {@link ContextMenuManager.Delegate} for NTP top-sites widget management. When
 * implemented by a tile's interaction delegate, the long-press context menu will prepend "Add site"
 * and append "Show frequently visited / Show shortcuts / Hide widget" (below a divider) to the
 * standard Chromium items.
 */
@NullMarked
public interface BraveNtpDelegate extends ContextMenuManager.Delegate {
    // Menu item IDs for the Brave-specific NTP entries, defined relative to
    // ContextMenuItemId.NUM_ENTRIES so they can never collide with a current or future Chromium
    // item, even if Chromium adds more entries.
    int BRAVE_ADD_SITE = ContextMenuManager.ContextMenuItemId.NUM_ENTRIES;
    int BRAVE_SHOW_FREQUENT = BRAVE_ADD_SITE + 1;
    int BRAVE_SHOW_SHORTCUTS = BRAVE_SHOW_FREQUENT + 1;
    int BRAVE_HIDE_WIDGET = BRAVE_SHOW_SHORTCUTS + 1;

    /**
     * Returns whether the given Brave-specific menu item should be shown.
     *
     * @param menuItemId One of {@link #BRAVE_ADD_SITE}, {@link #BRAVE_SHOW_FREQUENT}, {@link
     *     #BRAVE_SHOW_SHORTCUTS}, {@link #BRAVE_HIDE_WIDGET}.
     */
    boolean isBraveItemSupported(int menuItemId);

    /** Adds a new custom site shortcut to the NTP top-sites widget. */
    void braveAddSite();

    /** Switches the NTP top-sites widget to "Show frequently visited" mode. */
    void braveShowFrequent();

    /** Switches the NTP top-sites widget to "Show shortcuts" mode. */
    void braveShowShortcuts();

    /** Hides the NTP top-sites widget. */
    void braveHideWidget();

    /**
     * Returns the current top-sites display mode (0 = shortcuts, 1 = frequent), used to show a
     * checkmark next to the active mode option.
     */
    int getBraveTopSitesDisplayMode();

    /**
     * Returns the drawable resource ID to use as an end-icon on the currently-selected mode item
     * (the filled-circle checkmark), or 0 for none.
     */
    int getSelectedModeEndIconRes();
}
