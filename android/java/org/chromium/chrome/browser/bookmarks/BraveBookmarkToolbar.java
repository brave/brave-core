/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MenuItem;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;

public class BraveBookmarkToolbar extends BookmarkToolbar {
    private int MENU_IMPORT_ID = 100;
    private int MENU_EXPORT_ID = 101;

    private BraveBookmarkDelegate mBraveBookmarkDelegate;

    public BraveBookmarkToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
        int menuSize = getMenu().size();
        getMenu()
                .add(R.id.normal_menu_group, MENU_IMPORT_ID, menuSize, R.string.import_bookmarks)
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
        getMenu()
                .add(R.id.normal_menu_group, MENU_EXPORT_ID, menuSize + 1,
                        R.string.export_bookmarks)
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        if (menuItem.getItemId() == MENU_IMPORT_ID) {
            if (mBraveBookmarkDelegate != null) {
                mBraveBookmarkDelegate.importBookmarks();
            }
            return true;
        } else if (menuItem.getItemId() == MENU_EXPORT_ID) {
            if (mBraveBookmarkDelegate != null) {
                mBraveBookmarkDelegate.exportBookmarks();
            }
            return true;
        }

        return super.onMenuItemClick(menuItem);
    }

    void setBraveBookmarkDelegate(OneshotSupplier<BookmarkDelegate> bookmarkDelegateSupplier) {
        bookmarkDelegateSupplier.onAvailable((bookmarkDelegate) -> {
            if (bookmarkDelegate instanceof BraveBookmarkDelegate) {
                mBraveBookmarkDelegate = (BraveBookmarkDelegate) bookmarkDelegate;
            }
        });
    }
}
