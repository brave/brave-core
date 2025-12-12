/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.browser_ui.widget.displaystyle.HorizontalDisplayStyle;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig.DisplayStyle;
import org.chromium.components.browser_ui.widget.displaystyle.VerticalDisplayStyle;

/** Unit tests for helper function in {@link BraveNewTabPage} class. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveNewTabPageUtilUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Test
    public void testIsInNarrowWindowOnTablet() {
        UiConfig uiConfig = Mockito.mock(UiConfig.class);

        UiConfig.DisplayStyle displayStyleWide =
                new DisplayStyle(HorizontalDisplayStyle.WIDE, VerticalDisplayStyle.REGULAR);
        when(uiConfig.getCurrentDisplayStyle()).thenReturn(displayStyleWide);

        assertTrue(
                "Brave overridden isInNarrowWindowOnTablet on tablet when displayStyleWide =="
                        + " HorizontalDisplayStyle.WIDE.",
                BraveNewTabPageLayout.isInNarrowWindowOnTablet(true, uiConfig));
    }
}
