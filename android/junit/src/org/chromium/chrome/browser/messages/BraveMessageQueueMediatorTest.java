/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.messages;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.UserDataHost;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.RobolectricUtil;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.messages.ManagedMessageDispatcher;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Unit tests for {@link BraveMessageQueueMediator}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveMessageQueueMediatorTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BrowserControlsManager mBrowserControlsManager;
    @Mock private MessageContainerCoordinator mMessageContainerCoordinator;
    @Mock private LayoutStateProvider mLayoutStateProvider;
    @Mock private ManagedMessageDispatcher mMessageDispatcher;
    @Mock private ModalDialogManager mModalDialogManager;
    @Mock private Tab mTab;
    @Mock private ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    @Mock private BottomSheetController mBottomSheetController;

    private BraveMessageQueueMediator mMediator;
    private final ActivityTabProvider mActivityTabProvider = new ActivityTabProvider();

    @Before
    public void setUp() {
        OneshotSupplierImpl<LayoutStateProvider> layoutStateProviderSupplier =
                new OneshotSupplierImpl<>();
        SettableNonNullObservableSupplier<ModalDialogManager> modalDialogManagerSupplier =
                ObservableSuppliers.createNonNull(mModalDialogManager);
        mMediator =
                new BraveMessageQueueMediator(
                        mBrowserControlsManager,
                        mMessageContainerCoordinator,
                        mActivityTabProvider,
                        layoutStateProviderSupplier,
                        modalDialogManagerSupplier,
                        mBottomSheetController,
                        mActivityLifecycleDispatcher,
                        mMessageDispatcher);
        layoutStateProviderSupplier.set(mLayoutStateProvider);
        RobolectricUtil.runAllBackgroundAndUi();
    }

    /**
     * When top controls height is zero (e.g. bottom address bar), areBrowserControlsReady() must
     * return true — there are no top controls to wait for.
     */
    @Test
    public void testAreBrowserControlsReadyWhenTopControlsHeightIsZero() {
        mActivityTabProvider.setForTesting(mTab);
        when(mTab.isDestroyed()).thenReturn(false);
        when(mBrowserControlsManager.getTopControlsHeight()).thenReturn(0);

        assertTrue(
                "Should be ready when top controls height is zero",
                mMediator.areBrowserControlsReady());
    }

    /**
     * When top controls height is zero but the mediator has been destroyed, areBrowserControlsReady
     * must return false.
     */
    @Test
    public void testAreBrowserControlsReadyReturnsFalseWhenDestroyedWithZeroTopControlsHeight() {
        when(mBrowserControlsManager.getTopControlsHeight()).thenReturn(0);
        mMediator.destroy();

        assertFalse(
                "Should not be ready when mediator is destroyed",
                mMediator.areBrowserControlsReady());
    }

    /**
     * When top controls height is non-zero, areBrowserControlsReady() must fall back to the parent
     * class logic and return false when controls are not fully visible.
     */
    @Test
    public void testAreBrowserControlsReadyFallsBackToParentWhenTopControlsHeightIsNonZero() {
        mActivityTabProvider.setForTesting(mTab);
        when(mTab.isDestroyed()).thenReturn(false);
        // Provide a UserDataHost so TabBrowserControlsConstraintsHelper does not NPE.
        when(mTab.getUserDataHost()).thenReturn(new UserDataHost());
        when(mBrowserControlsManager.getTopControlsHeight()).thenReturn(56);
        // Controls are partially scrolled away — not fully visible.
        when(mBrowserControlsManager.getTopControlHiddenRatio()).thenReturn(0.5f);

        assertFalse(
                "Should fall back to parent logic and return false when controls are not visible",
                mMediator.areBrowserControlsReady());
    }
}
