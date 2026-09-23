/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.lenient;

import android.view.View;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker.LayerType;
import org.chromium.components.browser_ui.widget.scrim.ScrimManager;
import org.chromium.components.browser_ui.widget.scrim.ScrimProperties;
import org.chromium.ui.modelutil.PropertyModel;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveLocationBarFocusScrimHandlerUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private View mScrimTarget;
    @Mock private Runnable mClickDelegate;
    @Mock private LocationBarDataProvider mLocationBarDataProvider;
    @Mock private ScrimManager mScrimManager;
    @Mock private BottomControlsStacker mBottomControlsStacker;

    @Test
    public void testFocusScrim_coversNavigationBarAndBottomChin() {
        // Catch a regression to using the bottom chin height as the margin.
        lenient()
                .doReturn(37)
                .when(mBottomControlsStacker)
                .getHeightFromLayerToBottom(LayerType.BOTTOM_CHIN);

        LocationBarFocusScrimHandler scrimHandler =
                new LocationBarFocusScrimHandler(
                        mScrimManager,
                        (visible) -> {},
                        ContextUtils.getApplicationContext(),
                        mLocationBarDataProvider,
                        mClickDelegate,
                        mScrimTarget,
                        ObservableSuppliers.createNonNull(0),
                        mBottomControlsStacker);
        PropertyModel scrimModel = scrimHandler.getScrimModelForTesting();

        assertTrue(scrimModel.get(ScrimProperties.AFFECTS_NAVIGATION_BAR));

        scrimHandler.updateScrimVisualState();

        assertEquals(0, scrimModel.get(ScrimProperties.BOTTOM_MARGIN));
    }
}
