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

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.history.HistoryManagerUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.optional_button.BaseButtonDataProvider;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Handles displaying history button on toolbar. */
@NullMarked
public class BraveHistoryButtonController extends BaseButtonDataProvider {
    private final Context mContext;
    private final ObservableSupplier<Profile> mProfileSupplier;

    public BraveHistoryButtonController(
            Context context,
            Drawable buttonDrawable,
            ActivityTabProvider tabProvider,
            ObservableSupplier<Profile> profileSupplier,
            ModalDialogManager modalDialogManager) {
        super(
                tabProvider,
                modalDialogManager,
                buttonDrawable,
                context.getString(R.string.menu_history),
                /* actionChipLabelResId= */ Resources.ID_NULL,
                /* supportsTinting= */ true,
                /* iphCommandBuilder= */ null,
                AdaptiveToolbarButtonVariant.HISTORY,
                /* tooltipTextResId= */ R.string.menu_history);

        mContext = context;
        mProfileSupplier = profileSupplier;
    }

    @Override
    public void onClick(View view) {
        Profile profile = mProfileSupplier.get();
        if (profile == null) return;

        assert mContext instanceof Activity : "Context is not an Activity";
        if (!(mContext instanceof Activity)) return;

        Tab activeTab = mActiveTabSupplier.get();
        if (activeTab == null) return;

        HistoryManagerUtils.showHistoryManager((Activity) mContext, activeTab, profile);
    }

    @Override
    protected boolean shouldShowButton(@Nullable Tab tab) {
        if (!super.shouldShowButton(tab)) return false;

        // Show the `History` button for all tabs
        return true;
    }
}
