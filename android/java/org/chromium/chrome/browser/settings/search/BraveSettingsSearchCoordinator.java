/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.search;

import android.view.View;

import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.FragmentActivity;

import org.chromium.base.Callback;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.MultiColumnSettings;
import org.chromium.chrome.browser.settings.SettingsContainmentHelper;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;

/** Brave's {@link SettingsSearchCoordinator}, substituted for upstream's via plaster. */
@NullMarked
public class BraveSettingsSearchCoordinator extends SettingsSearchCoordinator {
    private final Toolbar mActionBar;

    public BraveSettingsSearchCoordinator(
            FragmentActivity activity,
            Toolbar actionBar,
            BooleanSupplier useMultiColumnSupplier,
            MultiColumnSettings multiColumnSettings,
            SettingsContainmentHelper containmentHelper,
            Profile profile,
            Callback<Integer> updateFirstVisibleTitle,
            MonotonicObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            boolean shownInTab) {
        super(
                activity,
                actionBar,
                useMultiColumnSupplier,
                multiColumnSettings,
                containmentHelper,
                profile,
                updateFirstVisibleTitle,
                modalDialogManagerSupplier,
                shownInTab);
        mActionBar = actionBar;
    }

    @Override
    public void showSearchBar(boolean show) {
        super.showSearchBar(show);
        // Closing a fragment's local search (e.g. Password Manager) hands its focus to the query
        // field, whose focus listener pops back to the search results while browsing them. Keep it
        // unfocusable meanwhile, but let a tap focus it so it still leads back to the results.
        View queryEdit = mActionBar.findViewById(R.id.search_query);
        if (queryEdit == null) return;
        if (show) {
            queryEdit.setFocusableInTouchMode(true);
        } else {
            queryEdit.setFocusable(false);
            queryEdit.setOnClickListener(BraveSettingsSearchCoordinator::focusOnClick);
        }
    }

    private static void focusOnClick(View queryEdit) {
        if (queryEdit.isFocusable()) return;
        queryEdit.setFocusableInTouchMode(true);
        queryEdit.requestFocus();
    }
}
