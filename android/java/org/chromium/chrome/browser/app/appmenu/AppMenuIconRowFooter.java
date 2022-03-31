/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.appmenu;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.graphics.drawable.DrawableCompat;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;

/**
 * A {@link LinearLayout} that displays a horizontal row of icons for page actions.
 */
public class AppMenuIconRowFooter extends LinearLayout implements View.OnClickListener {
    private AppMenuHandler mAppMenuHandler;
    private AppMenuDelegate mAppMenuDelegate;

    private ImageButton mForwardButton;
    private ImageButton mBookmarkButton;
    private ImageButton mDownloadButton;
    private ImageButton mPageInfoButton;
    private ImageButton mReloadButton;

    public AppMenuIconRowFooter(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mForwardButton = findViewById(R.id.forward_menu_id);
        mForwardButton.setOnClickListener(this);

        mBookmarkButton = findViewById(R.id.bookmark_this_page_id);
        mBookmarkButton.setOnClickListener(this);

        mDownloadButton = findViewById(R.id.offline_page_id);
        mDownloadButton.setOnClickListener(this);

        mPageInfoButton = findViewById(R.id.info_menu_id);
        mPageInfoButton.setOnClickListener(this);

        mReloadButton = findViewById(R.id.reload_menu_id);
        mReloadButton.setOnClickListener(this);

        // ImageView tinting doesn't work with LevelListDrawable, use Drawable tinting instead.
        // See https://crbug.com/891593 for details.
        Drawable icon = AppCompatResources.getDrawable(getContext(), R.drawable.btn_reload_stop);
        DrawableCompat.setTintList(icon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mReloadButton.setImageDrawable(icon);
    }

    /**
     * Initializes the icons, setting enabled state, drawables, and content descriptions.
     * @param appMenuHandler The {@link AppMenu} that contains the icon row.
     * @param bookmarkBridge The {@link BookmarkBridge} used to retrieve information about
     *                       bookmarks.
     * @param currentTab The current activity {@link Tab}.
     * @param appMenuDelegate The AppMenuDelegate to handle options item selection.
     */
    public void initialize(AppMenuHandler appMenuHandler, BookmarkBridge bookmarkBridge,
            Tab currentTab, AppMenuDelegate appMenuDelegate) {
        mAppMenuHandler = appMenuHandler;
        mAppMenuDelegate = appMenuDelegate;

        mForwardButton.setEnabled(currentTab.canGoForward());

        updateBookmarkMenuItem(bookmarkBridge, currentTab);

        mDownloadButton.setEnabled(DownloadUtils.isAllowedToDownloadPage(currentTab));

        loadingStateChanged(currentTab.isLoading());
    }

    @Override
    public void onClick(View v) {
        mAppMenuDelegate.onOptionsItemSelected(v.getId(), null);
        mAppMenuHandler.hideAppMenu();
    }

    /**
     * Called when the current tab's load state  has changed.
     * @param isLoading Whether the tab is currently loading.
     */
    public void loadingStateChanged(boolean isLoading) {
        mReloadButton.getDrawable().setLevel(isLoading
                        ? getResources().getInteger(R.integer.reload_button_level_stop)
                        : getResources().getInteger(R.integer.reload_button_level_reload));
        mReloadButton.setContentDescription(isLoading
                        ? getContext().getString(R.string.accessibility_btn_stop_loading)
                        : getContext().getString(R.string.accessibility_btn_refresh));
    }

    private void updateBookmarkMenuItem(BookmarkBridge bookmarkBridge, Tab currentTab) {
        mBookmarkButton.setEnabled(bookmarkBridge.isEditBookmarksEnabled());

        if (bookmarkBridge.hasBookmarkIdForTab(currentTab)) {
            mBookmarkButton.setImageResource(R.drawable.btn_star_filled);
            mBookmarkButton.setContentDescription(getContext().getString(R.string.edit_bookmark));
            ApiCompatibilityUtils.setImageTintList(mBookmarkButton,
                    AppCompatResources.getColorStateList(
                            getContext(), R.color.default_icon_color_accent1_tint_list));
        } else {
            mBookmarkButton.setImageResource(R.drawable.btn_star);
            mBookmarkButton.setContentDescription(
                    getContext().getString(R.string.accessibility_menu_bookmark));
        }
    }
}
