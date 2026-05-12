/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.browser_controls.BrowserControlsVisibilityManager;
import org.chromium.chrome.browser.glic.GlicButtonDelegate;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab_group_suggestion.toolbar.GroupSuggestionsButtonController;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabstrip.StripVisibilityState;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarBehavior;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTask;

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

    private final Activity mActivity;

    public BraveTabbedAdaptiveToolbarBehavior(
            Activity activity,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            Supplier<@Nullable TabCreatorManager> tabCreatorManagerSupplier,
            Supplier<@Nullable TabBookmarker> tabBookmarkerSupplier,
            NullableObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            ActivityTabProvider activityTabProvider,
            Runnable registerVoiceSearchRunnable,
            Supplier<GroupSuggestionsButtonController> groupSuggestionsButtonController,
            Supplier<@Nullable TabModelSelector> tabModelSelectorSupplier,
            MonotonicObservableSupplier<@StripVisibilityState Integer> tabStripVisibilitySupplier,
            GlicButtonDelegate toggleGlicCallback,
            Supplier<@Nullable ChromeAndroidTask> chromeAndroidTaskSupplier,
            BrowserControlsVisibilityManager browserControlsVisibilityManager) {
        super(
                activity,
                activityLifecycleDispatcher,
                tabCreatorManagerSupplier,
                tabBookmarkerSupplier,
                bookmarkModelSupplier,
                activityTabProvider,
                registerVoiceSearchRunnable,
                groupSuggestionsButtonController,
                tabModelSelectorSupplier,
                tabStripVisibilitySupplier,
                toggleGlicCallback,
                chromeAndroidTaskSupplier,
                browserControlsVisibilityManager);
        mActivity = activity;
    }

    @Override
    public int resultFilter(List<Integer> segmentationResults) {
        int result = AdaptiveToolbarBehavior.defaultResultFilter(mActivity, segmentationResults);
        if (result == AdaptiveToolbarButtonVariant.UNKNOWN) {
            maybeAddBraveButtonVariants();
            result = AdaptiveToolbarBehavior.defaultResultFilter(mActivity, segmentationResults);
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
