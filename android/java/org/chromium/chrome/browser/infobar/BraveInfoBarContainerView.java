/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.infobar;

import android.content.Context;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.components.infobars.InfoBar;
import org.chromium.components.infobars.InfoBarLayout;
import org.chromium.components.infobars.R;

public class BraveInfoBarContainerView extends InfoBarContainerView {
    // To be deleted in bytecode and super field to be used
    @Nullable private MonotonicObservableSupplier<EdgeToEdgeController> mEdgeToEdgeSupplier;

    BraveInfoBarContainerView(
            @NonNull Context context,
            @NonNull ContainerViewObserver containerViewObserver,
            @Nullable BrowserControlsStateProvider browserControlsStateProvider,
            @Nullable MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            boolean isTablet) {
        super(
                context,
                containerViewObserver,
                browserControlsStateProvider,
                edgeToEdgeSupplier,
                isTablet);
    }

    @Override
    void addInfoBar(InfoBar infoBar) {
        super.addInfoBar(infoBar);

        int infoBarIdentifier = (int) infoBar.getInfoBarIdentifier();
        if (infoBarIdentifier != BraveInfoBarIdentifier.NEW_TAB_TAKEOVER_INFOBAR_DELEGATE
                && infoBarIdentifier
                        != BraveInfoBarIdentifier.SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE) {
            return;
        }
        boolean drawEdgeToEdge =
                mEdgeToEdgeSupplier != null
                        && mEdgeToEdgeSupplier.get() != null
                        && mEdgeToEdgeSupplier.get().isDrawingToEdge();
        View infoBarView = infoBar.getView();
        if (!drawEdgeToEdge || !(infoBarView instanceof InfoBarLayout)) {
            return;
        }

        InfoBarLayout layout = (InfoBarLayout) infoBarView;
        TextView messageTexView = layout.getMessageTextView();
        // Adding extra padding at the bottom when edge to edge
        // is active.
        // See https://github.com/brave/brave-browser/issues/46513
        // for more info.
        messageTexView.setPadding(
                messageTexView.getPaddingLeft(),
                messageTexView.getPaddingTop(),
                messageTexView.getPaddingRight(),
                messageTexView.getPaddingBottom()
                        + messageTexView
                                .getResources()
                                .getDimensionPixelOffset(R.dimen.infobar_padding));
    }
}
