/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.chromium.chrome.browser.tasks.tab_management.TabManagementModuleProvider.SYNTHETIC_TRIAL_POSTFIX;

import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.ui.widget.ChromeImageView;

public class BraveTabGroupUiCoordinator extends TabGroupUiCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private TabGroupUiToolbarView mToolbarView;

    public BraveTabGroupUiCoordinator(ViewGroup parentView, ThemeColorProvider themeColorProvider,
            ScrimCoordinator scrimCoordinator,
            ObservableSupplier<Boolean> omniboxFocusStateSupplier) {
        super(parentView, themeColorProvider, scrimCoordinator, omniboxFocusStateSupplier);

        assert mToolbarView != null : "Make sure mToolbarView is properly patched in bytecode.";
        mToolbarView.setBackgroundColor(ApiCompatibilityUtils.getColor(
                mToolbarView.getResources(), R.color.toolbar_background_primary));
        ChromeImageView fadingEdgeStart =
                mToolbarView.findViewById(R.id.tab_strip_fading_edge_start);
        assert fadingEdgeStart != null : "Something has changed in upstream.";
        if (fadingEdgeStart != null) {
            fadingEdgeStart.setVisibility(View.GONE);
        }
        ChromeImageView fadingEdgeEnd = mToolbarView.findViewById(R.id.tab_strip_fading_edge_end);
        assert fadingEdgeEnd != null : "Something has changed in upstream.";
        if (fadingEdgeEnd != null) {
            fadingEdgeEnd.setVisibility(View.GONE);
        }
        ChromeImageView toolbarRightButton = mToolbarView.findViewById(R.id.toolbar_right_button);
        assert toolbarRightButton != null : "Something has changed in upstream.";
        if (toolbarRightButton != null) {
            toolbarRightButton.setImageResource(R.drawable.brave_new_group_tab);
        }
    }
}
