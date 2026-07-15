/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.favicon;

import android.app.Activity;
import android.graphics.drawable.Drawable;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider.ResourceTabFavicon;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider.StaticTabFaviconType;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider.TabFavicon;
import org.chromium.chrome.browser.tab_ui.TabListMode;
import org.chromium.ui.util.ColorUtils;

/** Unit tests for {@link BraveTabListFaviconProvider}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveTabListFaviconProviderTest {
    private Activity mActivity;

    @Before
    public void setUp() {
        mActivity = Robolectric.setupActivity(Activity.class);
        mActivity.setTheme(org.chromium.chrome.R.style.Theme_BrowserUI_DayNight);
        ColorUtils.setInNightModeForTesting(false);
    }

    @After
    public void tearDown() {
        ColorUtils.setInNightModeForTesting(null);
    }

    @Test
    public void testGetRoundedChromeFaviconNonTabStripReturnsBraveNtpFavicon() {
        TabFavicon favicon = createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false);

        assertBraveNtpFavicon(favicon);
    }

    @Test
    public void testGetRoundedChromeFaviconUsesDistinctLightAndDarkBraveVariants() {
        TabFavicon lightFavicon =
                createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false);

        assertBraveNtpFavicon(lightFavicon);
        Assert.assertEquals(
                lightFavicon,
                createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false));

        ColorUtils.setInNightModeForTesting(true);
        TabFavicon darkFavicon =
                createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false);

        assertBraveNtpFavicon(darkFavicon);
        Assert.assertEquals(
                darkFavicon,
                createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false));
        Assert.assertNotEquals(lightFavicon, darkFavicon);
    }

    @Test
    public void testGetRoundedChromeFaviconTabStripDelegatesToUpstream() {
        TabFavicon favicon = createProvider(/* isTabStrip= */ true).getRoundedChromeFavicon(false);

        Assert.assertTrue(favicon instanceof ResourceTabFavicon);
        Assert.assertEquals(
                new ResourceTabFavicon(
                        favicon.getDefaultDrawable(),
                        StaticTabFaviconType.ROUNDED_CHROME_FOR_STRIP),
                favicon);
    }

    @Test
    public void testBraveNtpFaviconHasUsableDefaultAndSelectedDrawablesInLightAndDarkModes() {
        assertBraveNtpFaviconHasUsableDefaultAndSelectedDrawables(/* inNightMode= */ false);
        assertBraveNtpFaviconHasUsableDefaultAndSelectedDrawables(/* inNightMode= */ true);
    }

    private void assertBraveNtpFaviconHasUsableDefaultAndSelectedDrawables(boolean inNightMode) {
        ColorUtils.setInNightModeForTesting(inNightMode);

        TabFavicon favicon = createProvider(/* isTabStrip= */ false).getRoundedChromeFavicon(false);

        assertBraveNtpFavicon(favicon);
        assertUsableDrawable(favicon.getDefaultDrawable());
        assertUsableDrawable(favicon.getSelectedDrawable());
        Assert.assertNotSame(favicon.getDefaultDrawable(), favicon.getSelectedDrawable());
    }

    private BraveTabListFaviconProvider createProvider(boolean isTabStrip) {
        return new BraveTabListFaviconProvider(
                mActivity,
                isTabStrip ? TabListMode.BOTTOM_STRIP : TabListMode.GRID,
                org.chromium.chrome.R.dimen.default_favicon_corner_radius,
                /* tabWebContentsFaviconDelegate= */ null);
    }

    private static void assertBraveNtpFavicon(TabFavicon favicon) {
        Assert.assertFalse(favicon instanceof ResourceTabFavicon);
        Assert.assertTrue(favicon.isRecolorAllowed());
        Assert.assertTrue(favicon.hasSelectedState());
        Assert.assertNotNull(favicon.getDefaultDrawable());
        Assert.assertNotNull(favicon.getSelectedDrawable());
    }

    private static void assertUsableDrawable(Drawable drawable) {
        Assert.assertTrue(drawable.getIntrinsicWidth() > 0);
        Assert.assertTrue(drawable.getIntrinsicHeight() > 0);
    }
}
