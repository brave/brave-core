/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.jank_tracker.PlaceholderJankTracker;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.cc.input.BrowserControlsState;
import org.chromium.chrome.browser.magic_stack.ModuleRegistry;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabBuilder;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.ui.RootUiCoordinator;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.batch.BlankCTATabInitialStateRule;
import org.chromium.components.browser_ui.util.BrowserControlsVisibilityDelegate;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.test.util.TestThreadUtils;
import org.chromium.ui.OverscrollAction;
import org.chromium.ui.base.BackGestureEventSwipeEdge;
import org.chromium.ui.base.PageTransition;

@RunWith(BaseJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
public class BraveSwipeRefreshHandlerTest {
    @ClassRule
    public static ChromeTabbedActivityTestRule sActivityTestRule =
            new ChromeTabbedActivityTestRule();

    @Rule
    public BlankCTATabInitialStateRule mInitialStateRule =
            new BlankCTATabInitialStateRule(sActivityTestRule, false);

    @Before
    public void setUp() throws Exception {}

    private TabbedModeTabDelegateFactory createTabDelegateFactory() {
        BrowserControlsVisibilityDelegate visibilityDelegate =
                new BrowserControlsVisibilityDelegate(BrowserControlsState.BOTH) {};
        ChromeTabbedActivity cta = sActivityTestRule.getActivity();
        RootUiCoordinator rootUiCoordinator = cta.getRootUiCoordinatorForTesting();
        return new TabbedModeTabDelegateFactory(
                sActivityTestRule.getActivity(),
                visibilityDelegate,
                new ObservableSupplierImpl<ShareDelegate>(),
                null,
                () -> {},
                rootUiCoordinator.getBottomSheetController(),
                /* chromeActivityNativeDelegate= */ cta,
                /* isCustomTab= */ true,
                rootUiCoordinator.getBrowserControlsManager(),
                cta.getFullscreenManager(),
                /* tabCreatorManager= */ cta,
                cta::getTabModelSelector,
                cta.getCompositorViewHolderSupplier(),
                cta.getModalDialogManagerSupplier(),
                cta::getSnackbarManager,
                cta.getBrowserControlsManager(),
                cta.getActivityTabProvider(),
                cta.getLifecycleDispatcher(),
                cta.getWindowAndroid(),
                new PlaceholderJankTracker(),
                rootUiCoordinator.getToolbarManager()::getToolbar,
                null,
                null,
                rootUiCoordinator.getToolbarManager().getTabStripHeightSupplier(),
                new OneshotSupplierImpl<ModuleRegistry>());
    }

    @Test
    @SmallTest
    public void denyRefreshForLeo() throws Exception {
        boolean refreshed =
                loadUrlAttemptRefresh("chrome-untrusted://chat/", PageTransition.FROM_API);
        Assert.assertFalse(refreshed);
    }

    @Test
    @SmallTest
    public void allowRefreshForNonLeo() throws Exception {
        boolean refreshed = loadUrlAttemptRefresh("https://brave.com", PageTransition.TYPED);
        Assert.assertTrue(refreshed);
    }

    private boolean loadUrlAttemptRefresh(String url, int transition) throws Exception {
        boolean refreshed =
                TestThreadUtils.runOnUiThreadBlocking(
                        () -> {
                            try {
                                Tab tab =
                                        new TabBuilder(sActivityTestRule.getProfile(false))
                                                .setWindow(
                                                        sActivityTestRule
                                                                .getActivity()
                                                                .getWindowAndroid())
                                                .setDelegateFactory(createTabDelegateFactory())
                                                .setLaunchType(
                                                        TabLaunchType.FROM_LONGPRESS_BACKGROUND)
                                                .build();
                                tab.loadUrl(new LoadUrlParams(url, transition));
                                BraveSwipeRefreshHandler braveSwipeRefreshHandler =
                                        (BraveSwipeRefreshHandler)
                                                BraveSwipeRefreshHandler.from(tab);
                                return braveSwipeRefreshHandler.start(
                                        OverscrollAction.PULL_TO_REFRESH,
                                        0,
                                        0,
                                        BackGestureEventSwipeEdge.LEFT);
                            } catch (Exception ex) {
                                throw ex;
                            }
                        });
        return refreshed;
    }
}
