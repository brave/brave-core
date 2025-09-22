/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui;

import android.content.Context;

import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpener;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonController;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.chrome.browser.toolbar.adaptive.BraveBookmarksButtonController;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.Supplier;

/**
 * Brave-specific coordinator for adaptive toolbar UI components. Extends the base
 * AdaptiveToolbarUiCoordinator with Brave-specific functionality.
 */
@NullMarked
public class BraveAdaptiveToolbarUiCoordinator extends AdaptiveToolbarUiCoordinator {
    // Variables below will be removed in bytecode, variables from the parent class will be used
    // instead.
    @Nullable private Context mContext;
    @Nullable private ActivityTabProvider mActivityTabProvider;
    @Nullable private Supplier<ModalDialogManager> mModalDialogManagerSupplier;
    @Nullable private ObservableSupplier<Profile> mProfileSupplier;
    @Nullable private AdaptiveToolbarButtonController mAdaptiveToolbarButtonController;

    public BraveAdaptiveToolbarUiCoordinator(
            Context context,
            ActivityTabProvider activityTabProvider,
            Supplier<ModalDialogManager> modalDialogManagerSupplier) {
        super(context, activityTabProvider, modalDialogManagerSupplier);
    }

    /**
     * Initialize Brave-specific adaptive toolbar components.
     *
     * @param bookmarkManagerOpener The bookmark manager opener for the `Bookmarks` button.
     */
    public void initializeBrave(BookmarkManagerOpener bookmarkManagerOpener) {
        assert mContext != null
                        && mActivityTabProvider != null
                        && mModalDialogManagerSupplier != null
                        && mProfileSupplier != null
                : "Bytecode changes were not applied!";
        if (mContext == null
                || mActivityTabProvider == null
                || mModalDialogManagerSupplier == null
                || mProfileSupplier == null) {
            return;
        }
        assert mAdaptiveToolbarButtonController != null
                : "initializeBrave must be called after initialize!";
        if (mAdaptiveToolbarButtonController == null) {
            return;
        }
        var bookmarksButtonController =
                new BraveBookmarksButtonController(
                        mContext,
                        AppCompatResources.getDrawable(mContext, R.drawable.brave_menu_bookmarks),
                        mActivityTabProvider,
                        mProfileSupplier,
                        mModalDialogManagerSupplier.get(),
                        bookmarkManagerOpener);
        mAdaptiveToolbarButtonController.addButtonVariant(
                AdaptiveToolbarButtonVariant.BOOKMARKS, bookmarksButtonController);
    }
}
