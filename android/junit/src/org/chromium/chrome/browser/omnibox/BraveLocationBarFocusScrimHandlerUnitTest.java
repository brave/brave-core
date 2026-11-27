/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.lenient;

import android.view.View;

import org.junit.Before;
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
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker.LayerType;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.styles.OmniboxResourceProvider;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.components.browser_ui.widget.scrim.ScrimManager;
import org.chromium.components.browser_ui.widget.scrim.ScrimProperties;
import org.chromium.ui.modelutil.PropertyModel;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveLocationBarFocusScrimHandlerUnitTest {
    private static final int BOTTOM_CHIN_HEIGHT = 37;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Runnable mClickDelegate;
    @Mock private ScrimManager mScrimManager;
    @Mock private BottomControlsStacker mBottomControlsStacker;

    @Before
    public void setUp() {
        lenient()
                .doReturn(BOTTOM_CHIN_HEIGHT)
                .when(mBottomControlsStacker)
                .getHeightFromLayerToBottom(LayerType.BOTTOM_CHIN);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testFocusScrim_coversNavigationBarAndBottomChin() {
        PropertyModel scrimModel = createScrimModel();

        assertTrue(scrimModel.get(ScrimProperties.AFFECTS_NAVIGATION_BAR));
        assertEquals(0, scrimModel.get(ScrimProperties.BOTTOM_MARGIN));
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testFocusScrim_androidBottomBarKeepsUpstreamBehavior() {
        PropertyModel scrimModel = createScrimModel();

        assertFalse(scrimModel.get(ScrimProperties.AFFECTS_NAVIGATION_BAR));
        assertEquals(BOTTOM_CHIN_HEIGHT, scrimModel.get(ScrimProperties.BOTTOM_MARGIN));
    }

    private PropertyModel createScrimModel() {
        LocationBarFocusScrimHandler scrimHandler =
                new LocationBarFocusScrimHandler(
                        mScrimManager,
                        (visible) -> {},
                        ContextUtils.getApplicationContext(),
                        new OmniboxResourceProvider(
                                ContextUtils.getApplicationContext(),
                                BrandedColorScheme.APP_DEFAULT),
                        mClickDelegate,
                        new View(ContextUtils.getApplicationContext()),
                        ObservableSuppliers.createNonNull(0),
                        mBottomControlsStacker);
        scrimHandler.updateScrimVisualState();
        return scrimHandler.getScrimModelForTesting();
    }
}
