/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Unit tests for BraveUnifiedPanelHandler functionality.
 *
 * These tests verify:
 * - Domain extraction utilities used for blocked tracker favicon display
 * - Tab switching logic for visibility and styling
 * - Site name parsing for display
 */
// TODO: Get rid of most of these once we have a better way to handle domains.
@RunWith(BaseRobolectricTestRunner.class)
public final class BraveUnifiedPanelDomainUtilsTest {
    private Activity mActivity;
    private BraveUnifiedPanelHandler mHandler;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mHandler = new BraveUnifiedPanelHandler(mActivity);
    }

    @SmallTest
    @Test
    public void testGetRootDomain_nullInput_returnsNull() throws Exception {
        assertNull(invokeGetRootDomain(null));
    }

    @SmallTest
    @Test
    public void testGetRootDomain_simpleDomain_returnsSame() throws Exception {
        // Two-part domains should return unchanged
        assertEquals("google.com", invokeGetRootDomain("google.com"));
        assertEquals("brave.com", invokeGetRootDomain("brave.com"));
        assertEquals("example.org", invokeGetRootDomain("example.org"));
    }

    @SmallTest
    @Test
    public void testGetRootDomain_subdomain_returnsRoot() throws Exception {
        // Subdomains should be stripped to the root
        assertEquals("google.com", invokeGetRootDomain("ads.google.com"));
        assertEquals("google.com", invokeGetRootDomain("www.google.com"));
        assertEquals("brave.com", invokeGetRootDomain("cdn.brave.com"));
    }

    @SmallTest
    @Test
    public void testGetRootDomain_deepSubdomain_returnsRoot() throws Exception {
        // Multiple subdomain levels should still return root
        assertEquals("google.com", invokeGetRootDomain("tracking.ads.google.com"));
        assertEquals("example.org", invokeGetRootDomain("a.b.c.d.example.org"));
    }

    @SmallTest
    @Test
    public void testGetRootDomain_singlePart_returnsSame() throws Exception {
        // Single-part domains (rare but possible) should return unchanged
        assertEquals("localhost", invokeGetRootDomain("localhost"));
    }

    // ========================================================================
    // extractDomain() tests
    // ========================================================================

    @SmallTest
    @Test
    public void testExtractDomain_nullInput_returnsNull() throws Exception {
        assertNull(invokeExtractDomain(null));
    }

    @SmallTest
    @Test
    public void testExtractDomain_emptyInput_returnsNull() throws Exception {
        assertNull(invokeExtractDomain(""));
    }

    @SmallTest
    @Test
    public void testExtractDomain_fullUrl_returnsDomain() throws Exception {
        assertEquals("example.com", invokeExtractDomain("https://example.com/path/to/resource"));
        assertEquals("brave.com", invokeExtractDomain("https://brave.com"));
        assertEquals("www.google.com", invokeExtractDomain("https://www.google.com/search?q=test"));
    }

    @SmallTest
    @Test
    public void testExtractDomain_httpUrl_returnsDomain() throws Exception {
        assertEquals("example.com", invokeExtractDomain("http://example.com/page"));
    }

    @SmallTest
    @Test
    public void testExtractDomain_urlWithoutScheme_returnsDomain() throws Exception {
        // Should handle URLs without scheme by prepending https://
        assertEquals("example.com", invokeExtractDomain("example.com/path"));
    }

    @SmallTest
    @Test
    public void testExtractDomain_urlWithPort_returnsDomain() throws Exception {
        assertEquals("localhost", invokeExtractDomain("http://localhost:8080/api"));
        assertEquals("example.com", invokeExtractDomain("https://example.com:443/secure"));
    }

    @SmallTest
    @Test
    public void testExtractDomain_subdomain_returnsFullHost() throws Exception {
        // extractDomain returns the full host, not the root domain
        assertEquals("ads.google.com", invokeExtractDomain("https://ads.google.com/tracker.js"));
        assertEquals("cdn.brave.com", invokeExtractDomain("https://cdn.brave.com/assets/img.png"));
    }

    // ========================================================================
    // switchToTab() UI behavior tests
    // ========================================================================

    @SmallTest
    @Test
    public void testSwitchToTab_shieldsTab_showsShieldsContent() throws Exception {
        // Inflate layout and set up handler's view references
        View panelView = inflateAndSetupHandler();

        // Switch to shields tab (index 0)
        invokeSwitchToTab(0);

        // Verify shields content is visible
        View shieldsContent = panelView.findViewById(R.id.shields_tab_content);
        View siteSettingsContent = panelView.findViewById(R.id.site_settings_tab_content);

        assertEquals(
                "Shields content should be visible",
                View.VISIBLE,
                shieldsContent.getVisibility());
        assertEquals(
                "Site settings content should be gone",
                View.GONE,
                siteSettingsContent.getVisibility());
    }

    @SmallTest
    @Test
    public void testSwitchToTab_siteSettingsTab_showsSiteSettingsContent() throws Exception {
        // Inflate layout and set up handler's view references
        View panelView = inflateAndSetupHandler();

        // Switch to site settings tab (index 1)
        invokeSwitchToTab(1);

        // Verify site settings content is visible
        View shieldsContent = panelView.findViewById(R.id.shields_tab_content);
        View siteSettingsContent = panelView.findViewById(R.id.site_settings_tab_content);

        assertEquals(
                "Shields content should be gone",
                View.GONE,
                shieldsContent.getVisibility());
        assertEquals(
                "Site settings content should be visible",
                View.VISIBLE,
                siteSettingsContent.getVisibility());
    }

    @SmallTest
    @Test
    public void testSwitchToTab_shieldsTab_showsShieldsIndicator() throws Exception {
        View panelView = inflateAndSetupHandler();

        invokeSwitchToTab(0);

        View shieldsIndicator = panelView.findViewById(R.id.shields_tab_indicator);
        View siteSettingsIndicator = panelView.findViewById(R.id.site_settings_tab_indicator);

        assertEquals(
                "Shields indicator should be visible",
                View.VISIBLE,
                shieldsIndicator.getVisibility());
        assertEquals(
                "Site settings indicator should be gone",
                View.GONE,
                siteSettingsIndicator.getVisibility());
    }

    @SmallTest
    @Test
    public void testSwitchToTab_siteSettingsTab_showsSiteSettingsIndicator() throws Exception {
        View panelView = inflateAndSetupHandler();

        invokeSwitchToTab(1);

        View shieldsIndicator = panelView.findViewById(R.id.shields_tab_indicator);
        View siteSettingsIndicator = panelView.findViewById(R.id.site_settings_tab_indicator);

        assertEquals(
                "Shields indicator should be gone",
                View.GONE,
                shieldsIndicator.getVisibility());
        assertEquals(
                "Site settings indicator should be visible",
                View.VISIBLE,
                siteSettingsIndicator.getVisibility());
    }

    @SmallTest
    @Test
    public void testSwitchToTab_toggleBetweenTabs_correctState() throws Exception {
        View panelView = inflateAndSetupHandler();

        // Start at shields
        invokeSwitchToTab(0);
        assertEquals(
                View.VISIBLE,
                panelView.findViewById(R.id.shields_tab_content).getVisibility());

        // Switch to site settings
        invokeSwitchToTab(1);
        assertEquals(
                View.GONE,
                panelView.findViewById(R.id.shields_tab_content).getVisibility());
        assertEquals(
                View.VISIBLE,
                panelView.findViewById(R.id.site_settings_tab_content).getVisibility());

        // Switch back to shields
        invokeSwitchToTab(0);
        assertEquals(
                View.VISIBLE,
                panelView.findViewById(R.id.shields_tab_content).getVisibility());
        assertEquals(
                View.GONE,
                panelView.findViewById(R.id.site_settings_tab_content).getVisibility());
    }

    // ========================================================================
    // dpToPx() tests
    // ========================================================================

    @SmallTest
    @Test
    public void testDpToPx_zeroInput_returnsZero() throws Exception {
        assertEquals(0, invokeDpToPx(0));
    }

    @SmallTest
    @Test
    public void testDpToPx_positiveInput_returnsScaledValue() throws Exception {
        // The exact value depends on display density, but should be positive
        int result = invokeDpToPx(16);
        assertTrue("16dp should convert to positive px value", result > 0);
    }

    @SmallTest
    @Test
    public void testDpToPx_largerInput_returnsLargerValue() throws Exception {
        int small = invokeDpToPx(8);
        int large = invokeDpToPx(16);
        assertTrue("Larger dp should produce larger px", large > small);
    }

    // ========================================================================
    // Helper methods
    // ========================================================================

    /**
     * Inflates the panel layout and sets up the handler's internal view references
     * via reflection, simulating what setupViews() does.
     */
    private View inflateAndSetupHandler() throws Exception {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View panelView = inflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Set the popup view field
        setPrivateField("mPopupView", panelView);

        // Set up all the view references that setupViews() would set
        setPrivateField("mShieldsTabButton", panelView.findViewById(R.id.shields_tab_button));
        setPrivateField(
                "mSiteSettingsTabButton", panelView.findViewById(R.id.site_settings_tab_button));
        setPrivateField("mShieldsTabIndicator", panelView.findViewById(R.id.shields_tab_indicator));
        setPrivateField(
                "mSiteSettingsTabIndicator",
                panelView.findViewById(R.id.site_settings_tab_indicator));
        setPrivateField("mShieldsTabIcon", panelView.findViewById(R.id.shields_tab_icon));
        setPrivateField("mSiteSettingsTabIcon", panelView.findViewById(R.id.site_settings_tab_icon));
        setPrivateField("mShieldsTabText", panelView.findViewById(R.id.shields_tab_text));
        setPrivateField("mSiteSettingsTabText", panelView.findViewById(R.id.site_settings_tab_text));
        setPrivateField("mShieldsContent", panelView.findViewById(R.id.shields_tab_content));
        setPrivateField(
                "mSiteSettingsContent", panelView.findViewById(R.id.site_settings_tab_content));

        return panelView;
    }

    private void setPrivateField(String fieldName, Object value) throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField(fieldName);
        field.setAccessible(true);
        field.set(mHandler, value);
    }

    private String invokeGetRootDomain(String domain) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("getRootDomain", String.class);
        method.setAccessible(true);
        return (String) method.invoke(mHandler, domain);
    }

    private String invokeExtractDomain(String url) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("extractDomain", String.class);
        method.setAccessible(true);
        return (String) method.invoke(mHandler, url);
    }

    private void invokeSwitchToTab(int tabIndex) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("switchToTab", int.class);
        method.setAccessible(true);
        method.invoke(mHandler, tabIndex);
    }

    private int invokeDpToPx(int dp) throws Exception {
        Method method = BraveUnifiedPanelHandler.class.getDeclaredMethod("dpToPx", int.class);
        method.setAccessible(true);
        return (int) method.invoke(mHandler, dp);
    }
}
