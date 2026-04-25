/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.view.View;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpener;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.optional_button.BaseButtonDataProvider;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Handles displaying bookmarks button on toolbar. */
@NullMarked
public class BraveBookmarksButtonController extends BaseButtonDataProvider {
    private final Context mContext;
    private final MonotonicObservableSupplier<Profile> mProfileSupplier;
    private final BookmarkManagerOpener mBookmarkManagerOpener;

    public BraveBookmarksButtonController(
            Context context,
            Drawable buttonDrawable,
            ActivityTabProvider tabProvider,
            MonotonicObservableSupplier<Profile> profileSupplier,
            ModalDialogManager modalDialogManager,
            BookmarkManagerOpener bookmarkManagerOpener) {
        super(
                tabProvider,
                modalDialogManager,
                buttonDrawable,
                context.getString(R.string.menu_bookmarks),
                /* actionChipLabelResId= */ Resources.ID_NULL,
                /* supportsTinting= */ true,
                /* iphCommandBuilder= */ null,
                AdaptiveToolbarButtonVariant.BOOKMARKS,
                /* tooltipTextResId= */ R.string.menu_bookmarks);

        mContext = context;
        mProfileSupplier = profileSupplier;
        mBookmarkManagerOpener = bookmarkManagerOpener;
    }

    @Override
    public void onClick(View view) {
        Profile profile = mProfileSupplier.get();
        if (profile == null) return;

        assert mContext instanceof Activity : "Context is not an Activity";
        if (!(mContext instanceof Activity)) return;

        mBookmarkManagerOpener.showBookmarkManager(
                (Activity) mContext, mActiveTabSupplier.get(), profile, /* folderId= */ null);
    }

    @Override
    protected boolean shouldShowButton(@Nullable Tab tab) {
        if (!super.shouldShowButton(tab)) return false;

        // Show the `Bookmarks` button for all tabs
        return true;
    }
}
