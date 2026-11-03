/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;

import androidx.core.content.ContextCompat;

import com.google.android.material.color.MaterialColors;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowPopupMenu;

import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpener;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.LocationBarModel;

/** Unit tests for the bookmarks long-press menu in {@link BottomToolbarBookmarksMenu}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BottomToolbarBookmarksMenuTest {
    private static final int LIGHT_ICON_COLOR = 0xFF484B4E;
    private static final int NIGHT_ICON_COLOR = 0xFFC2C4CF;

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private NullableObservableSupplier<BookmarkModel> mBookmarkModelSupplier;
    @Mock private BookmarkModel mBookmarkModel;
    @Mock private LocationBarModel mLocationBarModel;
    @Mock private BookmarkManagerOpener mBookmarkManagerOpener;
    @Mock private Tab mTab;

    private Activity mActivity;
    private View mAnchor;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mAnchor = new View(mActivity);

        when(mBookmarkModelSupplier.get()).thenReturn(mBookmarkModel);
        when(mBookmarkModel.isEditBookmarksEnabled()).thenReturn(true);
        when(mLocationBarModel.getTab()).thenReturn(mTab);
    }

    @Test
    public void testPopupMenuStyle_dynamicColors_usesDynamicStyle() {
        assertEquals(
                R.style.BookmarkTabPopupMenuDynamic,
                BottomToolbarBookmarksMenu.getPopupMenuStyle(true));
    }

    @Test
    public void testPopupMenuStyle_noDynamicColors_usesDefaultStyle() {
        assertEquals(
                R.style.BookmarkTabPopupMenu, BottomToolbarBookmarksMenu.getPopupMenuStyle(false));
    }

    @Test
    public void testIconTint_dynamicColors_usesThemeOnSurface() {
        Integer tint = BottomToolbarBookmarksMenu.getIconTint(mActivity, true, false);

        assertNotNull(tint);
        assertNotEquals(0, (int) tint);
        assertEquals(MaterialColors.getColor(mActivity, R.attr.colorOnSurface, 0), (int) tint);
    }

    @Test
    public void testIconTint_dynamicColors_ignoresNightModeResourceColor() {
        Integer tint = BottomToolbarBookmarksMenu.getIconTint(mActivity, true, true);

        assertEquals(MaterialColors.getColor(mActivity, R.attr.colorOnSurface, 0), (int) tint);
    }

    @Test
    public void testIconTint_noDynamicColors_dayMode_keepsIconColor() {
        assertNull(BottomToolbarBookmarksMenu.getIconTint(mActivity, false, false));
    }

    @Test
    public void testIconTint_noDynamicColors_nightMode_dayConfigContext_usesDayResourceColor() {
        Integer tint = BottomToolbarBookmarksMenu.getIconTint(mActivity, false, true);

        assertEquals(LIGHT_ICON_COLOR, (int) tint);
    }

    @Test
    public void testIconTint_noDynamicColors_nightMode_resolvesFromThemedContext() {
        // The menu's night mode follows Brave's theme, which can differ from the context the
        // caller holds; the color must come from the context passed in.
        Configuration nightConfig = new Configuration(mActivity.getResources().getConfiguration());
        nightConfig.uiMode =
                (nightConfig.uiMode & ~Configuration.UI_MODE_NIGHT_MASK)
                        | Configuration.UI_MODE_NIGHT_YES;
        Context nightContext = mActivity.createConfigurationContext(nightConfig);

        Integer tint = BottomToolbarBookmarksMenu.getIconTint(nightContext, false, true);

        assertEquals(
                ContextCompat.getColor(nightContext, R.color.bookmark_menu_text_color), (int) tint);
        assertEquals(NIGHT_ICON_COLOR, (int) tint);
    }

    @Test
    public void testMenu_pageNotBookmarked_showsAddAndView() {
        when(mBookmarkModel.hasBookmarkIdForTab(mTab)).thenReturn(false);

        Menu menu = showMenu();

        assertTrue(menu.findItem(R.id.add_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.edit_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.delete_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.view_bookmarks).isVisible());
    }

    @Test
    public void testMenu_pageBookmarked_showsEditDeleteAndView() {
        when(mBookmarkModel.hasBookmarkIdForTab(mTab)).thenReturn(true);

        Menu menu = showMenu();

        assertFalse(menu.findItem(R.id.add_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.edit_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.delete_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.view_bookmarks).isVisible());
    }

    @Test
    public void testMenu_editingDisabled_showsOnlyView() {
        when(mBookmarkModel.hasBookmarkIdForTab(mTab)).thenReturn(true);
        when(mBookmarkModel.isEditBookmarksEnabled()).thenReturn(false);

        Menu menu = showMenu();

        assertFalse(menu.findItem(R.id.add_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.edit_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.delete_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.view_bookmarks).isVisible());
    }

    @Test
    public void testMenu_noCurrentTab_showsAddAndView() {
        when(mLocationBarModel.getTab()).thenReturn(null);

        Menu menu = showMenu();

        assertTrue(menu.findItem(R.id.add_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.edit_bookmark).isVisible());
        assertFalse(menu.findItem(R.id.delete_bookmark).isVisible());
        assertTrue(menu.findItem(R.id.view_bookmarks).isVisible());
    }

    private Menu showMenu() {
        BottomToolbarBookmarksMenu.show(
                mAnchor, mBookmarkModelSupplier, mLocationBarModel, mBookmarkManagerOpener);
        PopupMenu popup = ShadowPopupMenu.getLatestPopupMenu();
        assertNotNull(popup);
        return popup.getMenu();
    }
}
