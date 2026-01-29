/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.content.Context;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab_group_suggestion.toolbar.GroupSuggestionsButtonController;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabstrip.StripVisibilityState;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarBehavior;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.List;
import java.util.Set;
import java.util.function.Supplier;

/**
 * Brave-specific {@link TabbedAdaptiveToolbarBehavior} that allows Brave-only toolbar variants to
 * be selected by segmentation.
 */
@NullMarked
public class BraveTabbedAdaptiveToolbarBehavior extends TabbedAdaptiveToolbarBehavior {
    static Set<Integer> sBraveButtons =
            Set.of(
                    AdaptiveToolbarButtonVariant.BOOKMARKS,
                    AdaptiveToolbarButtonVariant.HISTORY,
                    AdaptiveToolbarButtonVariant.DOWNLOADS,
                    AdaptiveToolbarButtonVariant.LEO,
                    AdaptiveToolbarButtonVariant.WALLET);

    private final Context mContext;

    public BraveTabbedAdaptiveToolbarBehavior(
            Context context,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            Supplier<@Nullable TabCreatorManager> tabCreatorManagerSupplier,
            Supplier<TabBookmarker> tabBookmarkerSupplier,
            ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            ActivityTabProvider activityTabProvider,
            Runnable registerVoiceSearchRunnable,
            Supplier<GroupSuggestionsButtonController> groupSuggestionsButtonController,
            Supplier<TabModelSelector> tabModelSelectorSupplier,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            ObservableSupplier<@StripVisibilityState Integer> tabStripVisibilitySupplier) {
        super(
                context,
                activityLifecycleDispatcher,
                tabCreatorManagerSupplier,
                tabBookmarkerSupplier,
                bookmarkModelSupplier,
                activityTabProvider,
                registerVoiceSearchRunnable,
                groupSuggestionsButtonController,
                tabModelSelectorSupplier,
                modalDialogManagerSupplier,
                tabStripVisibilitySupplier);
        mContext = context;
    }

    @Override
    public int resultFilter(List<Integer> segmentationResults) {
        int result = AdaptiveToolbarBehavior.defaultResultFilter(mContext, segmentationResults);
        if (result == AdaptiveToolbarButtonVariant.UNKNOWN) {
            maybeAddBraveButtonVariants();
            result = AdaptiveToolbarBehavior.defaultResultFilter(mContext, segmentationResults);
        }
        return result;
    }

    private void maybeAddBraveButtonVariants() {
        for (int braveButton : sBraveButtons) {
            if (AdaptiveToolbarBehavior.sValidButtons.contains(braveButton)) {
                // Already contains at least one Brave button, nothing to do.
                return;
            }
        }
        AdaptiveToolbarBehavior.sValidButtons.addAll(sBraveButtons);
    }
}
