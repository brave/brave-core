/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;
import android.view.View.OnClickListener;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.tabmodel.TabGroupModelFilter;
import org.chromium.chrome.browser.ui.actions.ResourceButtonData;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.tab_ui.R;

import java.util.function.DoubleConsumer;
import java.util.function.Supplier;

@NullMarked
public class BraveIncognitoTabSwitcherPane extends IncognitoTabSwitcherPane {

    public BraveIncognitoTabSwitcherPane(
            Context context,
            TabSwitcherPaneCoordinatorFactory factory,
            Supplier<TabGroupModelFilter> incognitoTabGroupModelFilterSupplier,
            OnClickListener newTabButtonClickListener,
            @Nullable OneshotSupplier<IncognitoReauthController> incognitoReauthControllerSupplier,
            DoubleConsumer onToolbarAlphaChange,
            UserEducationHelper userEducationHelper,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            MonotonicObservableSupplier<CompositorViewHolder> compositorViewHolderSupplier,
            TabGroupCreationUiDelegate tabGroupCreationUiDelegate,
            NonNullObservableSupplier<Boolean> xrSpaceModeObservableSupplier) {
        super(
                context,
                factory,
                incognitoTabGroupModelFilterSupplier,
                newTabButtonClickListener,
                incognitoReauthControllerSupplier,
                onToolbarAlphaChange,
                userEducationHelper,
                edgeToEdgeSupplier,
                compositorViewHolderSupplier,
                tabGroupCreationUiDelegate,
                xrSpaceModeObservableSupplier);

        ResourceButtonData newReferenceButtonData =
                new ResourceButtonData(
                        R.string.accessibility_tab_switcher_incognito_stack,
                        R.string.accessibility_tab_switcher_incognito_stack,
                        R.drawable.brave_menu_new_private_tab);

        BraveReflectionUtil.setField(
                IncognitoTabSwitcherPane.class,
                "mReferenceButtonData",
                this,
                newReferenceButtonData);
    }
}
