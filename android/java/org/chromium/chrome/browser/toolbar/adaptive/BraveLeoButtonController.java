/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.view.View;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.optional_button.BaseButtonDataProvider;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Handles displaying Leo AI button on toolbar. */
@NullMarked
public class BraveLeoButtonController extends BaseButtonDataProvider {
    private final Context mContext;

    public BraveLeoButtonController(
            Context context,
            Drawable buttonDrawable,
            ActivityTabProvider tabProvider,
            ObservableSupplier<Profile> profileSupplier,
            ModalDialogManager modalDialogManager) {
        super(
                tabProvider,
                modalDialogManager,
                buttonDrawable,
                context.getString(R.string.menu_brave_leo),
                /* actionChipLabelResId= */ Resources.ID_NULL,
                /* supportsTinting= */ true,
                /* iphCommandBuilder= */ null,
                AdaptiveToolbarButtonVariant.LEO,
                /* tooltipTextResId= */ R.string.menu_brave_leo);

        mContext = context;
    }

    @Override
    public void onClick(View view) {
        assert mContext instanceof BraveActivity : "Context is not an BraveActivity";

        // Open Leo AI
        if (mContext instanceof BraveActivity braveActivity) {
            braveActivity.openBraveLeo();
        }
    }

    @Override
    protected boolean shouldShowButton(@Nullable Tab tab) {
        if (!super.shouldShowButton(tab)) return false;

        // Show the Leo AI button only if Leo is enabled and not in incognito mode
        return BraveLeoPrefUtils.isLeoEnabled() && (tab == null || !tab.isIncognito());
    }
}
