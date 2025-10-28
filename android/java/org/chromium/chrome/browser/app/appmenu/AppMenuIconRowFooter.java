/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.appmenu;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.graphics.drawable.DrawableCompat;

import com.google.android.material.button.MaterialButton;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
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

    private MaterialButton mForwardButton;
    private MaterialButton mBookmarkButton;
    private MaterialButton mDownloadButton;
    private MaterialButton mShareButton;
    private MaterialButton mHomeButton;
    private MaterialButton mReloadButton;

    public AppMenuIconRowFooter(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mForwardButton = findViewById(R.id.forward_menu_id);
        mForwardButton.setOnClickListener(this);
        Drawable forwardIcon = AppCompatResources.getDrawable(getContext(), R.drawable.btn_forward);
        DrawableCompat.setTintList(
                forwardIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mForwardButton.setIcon(forwardIcon);

        mBookmarkButton = findViewById(R.id.bookmark_this_page_id);
        mBookmarkButton.setOnClickListener(this);
        Drawable bookmarkIcon =
                AppCompatResources.getDrawable(getContext(), R.drawable.star_outline_24dp);
        DrawableCompat.setTintList(
                bookmarkIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mBookmarkButton.setIcon(bookmarkIcon);

        mDownloadButton = findViewById(R.id.offline_page_id);
        mDownloadButton.setOnClickListener(this);
        Drawable downloadIcon =
                AppCompatResources.getDrawable(
                        getContext(), R.drawable.ic_file_download_white_24dp);
        DrawableCompat.setTintList(
                downloadIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mDownloadButton.setIcon(downloadIcon);

        mShareButton = findViewById(R.id.share_menu_id);
        mShareButton.setOnClickListener(this);
        Drawable shareIcon = AppCompatResources.getDrawable(getContext(), R.drawable.share_icon);
        DrawableCompat.setTintList(
                shareIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mShareButton.setIcon(shareIcon);

        mHomeButton = findViewById(R.id.home_menu_id);
        mHomeButton.setOnClickListener(this);
        Drawable homeIcon =
                AppCompatResources.getDrawable(getContext(), R.drawable.btn_toolbar_home);
        DrawableCompat.setTintList(
                homeIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mHomeButton.setIcon(homeIcon);

        mReloadButton = findViewById(R.id.reload_menu_id);
        mReloadButton.setOnClickListener(this);
        Drawable reloadIcon =
                AppCompatResources.getDrawable(getContext(), R.drawable.btn_reload_stop);
        DrawableCompat.setTintList(
                reloadIcon,
                AppCompatResources.getColorStateList(
                        getContext(), R.color.default_icon_color_tint_list));
        mReloadButton.setIcon(reloadIcon);
    }

    /**
     * Initializes the icons, setting enabled state, drawables, and content descriptions.
     * @param appMenuHandler The {@link AppMenu} that contains the icon row.
     * @param bookmarkBridge The {@link BookmarkModel} used to retrieve information about
     *                       bookmarks.
     * @param currentTab The current activity {@link Tab}.
     * @param appMenuDelegate The AppMenuDelegate to handle options item selection.
     */
    public void initialize(AppMenuHandler appMenuHandler, BookmarkModel bookmarkBridge,
            @Nullable Tab currentTab, AppMenuDelegate appMenuDelegate) {
        mAppMenuHandler = appMenuHandler;
        mAppMenuDelegate = appMenuDelegate;

        mForwardButton.setEnabled(currentTab == null ? false : currentTab.canGoForward());

        updateBookmarkMenuItem(bookmarkBridge, currentTab);

        mDownloadButton.setEnabled(
                currentTab == null ? false : DownloadUtils.isAllowedToDownloadPage(currentTab));

        loadingStateChanged(currentTab == null ? false : currentTab.isLoading());
    }

    @Override
    public void onClick(View v) {
        mAppMenuDelegate.onOptionsItemSelected(v.getId(), null);
        mAppMenuHandler.hideAppMenu();
    }

    /**
     * Called when the current tab's load state has changed.
     *
     * @param isLoading Whether the tab is currently loading.
     */
    public void loadingStateChanged(boolean isLoading) {
        Drawable icon = mReloadButton.getIcon();
        if (icon != null) {
            icon.setLevel(
                    isLoading
                            ? getResources().getInteger(R.integer.reload_button_level_stop)
                            : getResources().getInteger(R.integer.reload_button_level_reload));
        }
        mReloadButton.setContentDescription(
                isLoading
                        ? getContext().getString(R.string.accessibility_btn_stop_loading)
                        : getContext().getString(R.string.accessibility_btn_refresh));
    }

    private void updateBookmarkMenuItem(BookmarkModel bookmarkBridge, @Nullable Tab currentTab) {
        mBookmarkButton.setEnabled(bookmarkBridge.isEditBookmarksEnabled());

        if (currentTab != null && bookmarkBridge.hasBookmarkIdForTab(currentTab)) {
            mBookmarkButton.setIcon(
                    AppCompatResources.getDrawable(getContext(), R.drawable.btn_star_filled));
            mBookmarkButton.setContentDescription(getContext().getString(R.string.edit_bookmark));
            mBookmarkButton.setIconTint(
                    AppCompatResources.getColorStateList(
                            getContext(), R.color.default_icon_color_accent1_tint_list));
        } else {
            mBookmarkButton.setIcon(
                    AppCompatResources.getDrawable(getContext(), R.drawable.star_outline_24dp));
            mBookmarkButton.setContentDescription(
                    getContext().getString(R.string.accessibility_menu_bookmark));
        }
    }
}
