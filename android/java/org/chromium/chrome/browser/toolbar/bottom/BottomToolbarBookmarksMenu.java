/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.ContextThemeWrapper;
import android.view.MenuItem;
import android.view.View;
import android.widget.PopupMenu;

import androidx.annotation.ColorInt;
import androidx.annotation.StyleRes;
import androidx.annotation.VisibleForTesting;
import androidx.core.content.ContextCompat;
import androidx.core.view.MenuCompat;

import com.google.android.material.color.MaterialColors;

import org.chromium.base.Log;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpener;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.theme.BraveDynamicColors;
import org.chromium.chrome.browser.toolbar.LocationBarModel;
import org.chromium.components.bookmarks.BookmarkId;

/** Long-press menu of the bottom toolbar's Bookmarks button. */
@NullMarked
public class BottomToolbarBookmarksMenu {
    private static final String TAG = "BookmarksMenu";

    private BottomToolbarBookmarksMenu() {}

    /**
     * Shows the menu anchored to {@code view}. Colors are resolved from the anchor's context: the
     * caller's context is the application context, which has no activity theme (dynamic colors) and
     * follows the system night mode rather than Brave's theme.
     */
    public static void show(
            View view,
            NullableObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            LocationBarModel locationBarModel,
            BookmarkManagerOpener bookmarkManagerOpener) {
        boolean useDynamicColors = BraveDynamicColors.isDynamicColorsEnabled();
        Context themedContext = view.getContext();
        Context wrapper =
                new ContextThemeWrapper(themedContext, getPopupMenuStyle(useDynamicColors));

        PopupMenu popup = new PopupMenu(wrapper, view);
        popup.getMenuInflater().inflate(R.menu.bookmark_tab_menu, popup.getMenu());
        popup.setForceShowIcon(true);

        Tab currentTab = locationBarModel.getTab();
        BookmarkModel bookmarkModel = bookmarkModelSupplier.get();
        boolean isBookmarked =
                currentTab != null
                        && bookmarkModel != null
                        && bookmarkModel.hasBookmarkIdForTab(currentTab);
        boolean editingAllowed =
                currentTab == null
                        || bookmarkModel == null
                        || bookmarkModel.isEditBookmarksEnabled();

        MenuCompat.setGroupDividerEnabled(popup.getMenu(), true);

        MenuItem addMenuItem = popup.getMenu().findItem(R.id.add_bookmark);
        MenuItem editMenuItem = popup.getMenu().findItem(R.id.edit_bookmark);
        MenuItem viewMenuItem = popup.getMenu().findItem(R.id.view_bookmarks);
        MenuItem deleteMenuItem = popup.getMenu().findItem(R.id.delete_bookmark);

        Integer tintColor =
                getIconTint(
                        themedContext,
                        useDynamicColors,
                        GlobalNightModeStateProviderHolder.getInstance().isInNightMode());
        if (tintColor != null) {
            tintIcon(addMenuItem.getIcon(), tintColor);
            tintIcon(editMenuItem.getIcon(), tintColor);
            tintIcon(viewMenuItem.getIcon(), tintColor);
            tintIcon(deleteMenuItem.getIcon(), tintColor);
        }

        if (editingAllowed) {
            if (isBookmarked) {
                addMenuItem.setVisible(false);
            } else {
                editMenuItem.setVisible(false);
                deleteMenuItem.setVisible(false);
            }
        } else {
            addMenuItem.setVisible(false);
            editMenuItem.setVisible(false);
            deleteMenuItem.setVisible(false);
        }

        popup.setOnMenuItemClickListener(
                item -> {
                    int id = item.getItemId();
                    BraveActivity activity = null;
                    try {
                        activity = BraveActivity.getBraveActivity();
                    } catch (BraveActivity.BraveActivityNotFoundException e) {
                        Log.e(TAG, "popup click " + e);
                    }
                    if (currentTab == null || activity == null) {
                        return false;
                    }

                    if (id == R.id.add_bookmark || id == R.id.delete_bookmark) {
                        activity.addOrEditBookmark(currentTab);
                        return true;
                    } else if (id == R.id.edit_bookmark && bookmarkModel != null) {
                        BraveActivity activityFinal = activity;
                        bookmarkModel.finishLoadingBookmarkModel(
                                () -> {
                                    BookmarkId bookmarkId =
                                            bookmarkModel.getUserBookmarkIdForTab(currentTab);
                                    if (bookmarkId != null) {
                                        bookmarkManagerOpener.startEditActivity(
                                                activityFinal, currentTab.getProfile(), bookmarkId);
                                    }
                                });
                        return true;
                    } else if (id == R.id.view_bookmarks) {
                        activity.showBookmarkManager(currentTab.getProfile(), currentTab);
                        return true;
                    }
                    return false;
                });

        popup.show();
    }

    @VisibleForTesting
    static @StyleRes int getPopupMenuStyle(boolean useDynamicColors) {
        return useDynamicColors
                ? R.style.BookmarkTabPopupMenuDynamic
                : R.style.BookmarkTabPopupMenu;
    }

    /** Returns the icon tint, or null when the icons keep their own (light theme) color. */
    @VisibleForTesting
    static @Nullable @ColorInt Integer getIconTint(
            Context themedContext, boolean useDynamicColors, boolean isInNightMode) {
        if (useDynamicColors) {
            return MaterialColors.getColor(themedContext, R.attr.colorOnSurface, 0);
        }
        if (isInNightMode) {
            return ContextCompat.getColor(themedContext, R.color.bookmark_menu_text_color);
        }
        return null;
    }

    private static void tintIcon(@Nullable Drawable icon, int color) {
        if (icon != null) icon.setTint(color);
    }
}
