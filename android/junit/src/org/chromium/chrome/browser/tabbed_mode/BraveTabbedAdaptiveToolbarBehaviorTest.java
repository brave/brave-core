/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static org.junit.Assert.assertEquals;

import android.content.Context;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;

import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab_group_suggestion.toolbar.GroupSuggestionsButtonController;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarBehavior;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.chrome.browser.toolbar.top.tab_strip.StripVisibilityState;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.Arrays;
import java.util.function.Supplier;

/** Unit tests for {@link BraveTabbedAdaptiveToolbarBehavior}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveTabbedAdaptiveToolbarBehaviorTest {
    private BraveTabbedAdaptiveToolbarBehavior mBehavior;

    @Before
    public void setUp() {
        Context context = RuntimeEnvironment.getApplication();
        AdaptiveToolbarBehavior.sValidButtons.clear();

        ObservableSupplierImpl<BookmarkModel> bookmarkModelSupplier =
                new ObservableSupplierImpl<>();
        ObservableSupplierImpl<Integer> tabStripVisibilitySupplier = new ObservableSupplierImpl<>();
        tabStripVisibilitySupplier.set(StripVisibilityState.VISIBLE);

        ActivityLifecycleDispatcher lifecycleDispatcher =
                Mockito.mock(ActivityLifecycleDispatcher.class);
        Supplier<@Nullable TabCreatorManager> tabCreatorManagerSupplier = () -> null;
        Supplier<GroupSuggestionsButtonController> groupSuggestionsControllerSupplier = () -> null;
        Supplier<TabModelSelector> tabModelSelectorSupplier = () -> null;
        Supplier<ModalDialogManager> modalDialogManagerSupplier = () -> null;

        mBehavior =
                new BraveTabbedAdaptiveToolbarBehavior(
                        context,
                        lifecycleDispatcher,
                        tabCreatorManagerSupplier,
                        () -> null,
                        bookmarkModelSupplier,
                        new ActivityTabProvider(),
                        () -> {},
                        groupSuggestionsControllerSupplier,
                        tabModelSelectorSupplier,
                        modalDialogManagerSupplier,
                        tabStripVisibilitySupplier);
    }

    @After
    public void tearDown() {
        AdaptiveToolbarBehavior.sValidButtons.clear();
    }

    @Test
    public void resultFilter_returnsBraveVariantWhenDefaultIsUnknown() {
        int result =
                mBehavior.resultFilter(
                        Arrays.asList(AdaptiveToolbarButtonVariant.BOOKMARKS));

        assertEquals(AdaptiveToolbarButtonVariant.BOOKMARKS, result);
    }

    @Test
    public void resultFilter_keepsCommonVariantPriority() {
        int result =
                mBehavior.resultFilter(
                        Arrays.asList(
                                AdaptiveToolbarButtonVariant.SHARE,
                                AdaptiveToolbarButtonVariant.HISTORY));

        assertEquals(AdaptiveToolbarButtonVariant.SHARE, result);
    }
}
