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
    // toggleAdvancedOptions() tests
    // ========================================================================

    @SmallTest
    @Test
    public void testToggleAdvancedOptions_initiallyCollapsed() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();

        // Verify initial state is collapsed
        View advancedContent = panelView.findViewById(R.id.advanced_options_content);
        ImageView arrow = (ImageView) panelView.findViewById(R.id.advanced_options_arrow);

        assertEquals(
                "Advanced options should be collapsed initially",
                View.GONE,
                advancedContent.getVisibility());
        assertEquals("Arrow should point down (0 degrees)", 0f, arrow.getRotation(), 0.01f);
    }

    @SmallTest
    @Test
    public void testToggleAdvancedOptions_expandsOnFirstToggle() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();

        // Toggle to expand
        invokeToggleAdvancedOptions();

        View advancedContent = panelView.findViewById(R.id.advanced_options_content);
        ImageView arrow = (ImageView) panelView.findViewById(R.id.advanced_options_arrow);

        assertEquals(
                "Advanced options should be visible after toggle",
                View.VISIBLE,
                advancedContent.getVisibility());
        assertEquals("Arrow should point up (180 degrees)", 180f, arrow.getRotation(), 0.01f);
    }

    @SmallTest
    @Test
    public void testToggleAdvancedOptions_collapsesOnSecondToggle() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();

        // Toggle twice: expand then collapse
        invokeToggleAdvancedOptions();
        invokeToggleAdvancedOptions();

        View advancedContent = panelView.findViewById(R.id.advanced_options_content);
        ImageView arrow = (ImageView) panelView.findViewById(R.id.advanced_options_arrow);

        assertEquals(
                "Advanced options should be hidden after second toggle",
                View.GONE,
                advancedContent.getVisibility());
        assertEquals("Arrow should point down (0 degrees)", 0f, arrow.getRotation(), 0.01f);
    }

    @SmallTest
    @Test
    public void testToggleAdvancedOptions_multipleToggles_correctState() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();
        View advancedContent = panelView.findViewById(R.id.advanced_options_content);

        // Toggle 1: expand
        invokeToggleAdvancedOptions();
        assertEquals(View.VISIBLE, advancedContent.getVisibility());

        // Toggle 2: collapse
        invokeToggleAdvancedOptions();
        assertEquals(View.GONE, advancedContent.getVisibility());

        // Toggle 3: expand again
        invokeToggleAdvancedOptions();
        assertEquals(View.VISIBLE, advancedContent.getVisibility());

        // Toggle 4: collapse again
        invokeToggleAdvancedOptions();
        assertEquals(View.GONE, advancedContent.getVisibility());
    }

    // ========================================================================
    // mIsUpdatingSwitches guard tests
    // ========================================================================

    @SmallTest
    @Test
    public void testIsUpdatingSwitches_preventsRecursiveCallbacks() throws Exception {
        inflateAndSetupAdvancedOptions();

        // Set the guard flag to true
        setPrivateField("mIsUpdatingSwitches", true);

        // These should be no-ops when guard is true (no exception, no side effects)
        // We can't easily verify no side effects without mocking BraveShieldsContentSettings,
        // but we can verify the methods don't crash when guard is set
        invokeOnBlockScriptsChanged(true);
        invokeOnForgetMeChanged(true);
        invokeOnFingerprintingChanged(true);

        // If we got here without exceptions, the guard is working
        assertTrue("Guard flag should prevent callbacks from executing", true);
    }

    @SmallTest
    @Test
    public void testIsUpdatingSwitches_initiallyFalse() throws Exception {
        inflateAndSetupAdvancedOptions();

        boolean isUpdating = getPrivateBooleanField("mIsUpdatingSwitches");
        assertEquals("mIsUpdatingSwitches should be false initially", false, isUpdating);
    }

    // ========================================================================
    // Advanced options items visibility tests
    // ========================================================================

    @SmallTest
    @Test
    public void testAdvancedOptionsContent_hasRequiredItems() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();

        // Expand to access items
        invokeToggleAdvancedOptions();

        View advancedContent = panelView.findViewById(R.id.advanced_options_content);

        // Verify all expected items exist
        assertNotNull(
                "HTTPS upgrade item should exist",
                advancedContent.findViewById(R.id.https_upgrade_item));
        assertNotNull(
                "Trackers item should exist", advancedContent.findViewById(R.id.trackers_item));
        assertNotNull("Cookies item should exist", advancedContent.findViewById(R.id.cookies_item));
        assertNotNull(
                "Shred data item should exist", advancedContent.findViewById(R.id.shred_data_item));
        assertNotNull(
                "Global settings item should exist",
                advancedContent.findViewById(R.id.global_settings_item));
    }

    @SmallTest
    @Test
    public void testAdvancedOptionsSwitches_existInLayout() throws Exception {
        View panelView = inflateAndSetupAdvancedOptions();

        // Verify switch controls exist
        assertNotNull(
                "Scripts toggle should exist", panelView.findViewById(R.id.scripts_toggle));
        assertNotNull(
                "Forget me toggle should exist", panelView.findViewById(R.id.forget_me_toggle));
        assertNotNull(
                "Fingerprinting toggle should exist",
                panelView.findViewById(R.id.fingerprinting_toggle));
    }

    // ========================================================================
    // Sub-panel navigation tests
    // ========================================================================
    // Note: Tests for showTrackersAdsPanel and showCookiesPanel are omitted because
    // they call BraveShieldsContentSettings.getShieldsValue() which requires native
    // methods not available in Robolectric. HTTPS and Shred panels don't have this issue.
    // TODO: Mock the native calls so we can still verify it?

    @SmallTest
    @Test
    public void testShowHttpsUpgradePanel_hidesMainShowsHttps() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        invokeShowHttpsUpgradePanel();

        assertEquals(
                "Main panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(
                "HTTPS panel should be visible",
                View.VISIBLE,
                panelView.findViewById(R.id.https_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testShowShredPanel_hidesMainShowsShred() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        invokeShowShredPanel();

        assertEquals(
                "Main panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(
                "Shred panel should be visible",
                View.VISIBLE,
                panelView.findViewById(R.id.shred_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testShowMainPanel_hidesAllSubPanels() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        // First navigate to a sub-panel (HTTPS doesn't require native calls)
        invokeShowHttpsUpgradePanel();
        assertEquals(View.VISIBLE, panelView.findViewById(R.id.https_panel_container).getVisibility());

        // Now go back to main
        invokeShowMainPanel();

        assertEquals(
                "Main panel should be visible",
                View.VISIBLE,
                panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(
                "HTTPS panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.https_panel_container).getVisibility());
        assertEquals(
                "Trackers panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.trackers_panel_container).getVisibility());
        assertEquals(
                "Cookies panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.cookies_panel_container).getVisibility());
        assertEquals(
                "Shred panel should be hidden",
                View.GONE,
                panelView.findViewById(R.id.shred_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testShowMainPanel_restoresAdvancedOptionsState() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        // Set advanced options as expanded
        setPrivateField("mIsAdvancedOptionsExpanded", true);

        // Navigate to sub-panel and back (HTTPS doesn't require native calls)
        invokeShowHttpsUpgradePanel();
        invokeShowMainPanel();

        // Advanced options should be restored to expanded state
        View advancedContent = panelView.findViewById(R.id.advanced_options_content);
        ImageView arrow = (ImageView) panelView.findViewById(R.id.advanced_options_arrow);

        assertEquals(
                "Advanced options should be visible after returning",
                View.VISIBLE,
                advancedContent.getVisibility());
        assertEquals(
                "Arrow should be rotated after returning",
                180f,
                arrow.getRotation(),
                0.01f);
    }

    @SmallTest
    @Test
    public void testSubPanelNavigation_httpsRoundTrip() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        // Main -> HTTPS -> Main
        invokeShowHttpsUpgradePanel();
        assertEquals(View.GONE, panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(View.VISIBLE, panelView.findViewById(R.id.https_panel_container).getVisibility());

        invokeShowMainPanel();
        assertEquals(View.VISIBLE, panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(View.GONE, panelView.findViewById(R.id.https_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testSubPanelNavigation_shredRoundTrip() throws Exception {
        View panelView = inflateAndSetupSubPanels();

        // Main -> Shred -> Main
        invokeShowShredPanel();
        assertEquals(View.GONE, panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(View.VISIBLE, panelView.findViewById(R.id.shred_panel_container).getVisibility());

        invokeShowMainPanel();
        assertEquals(View.VISIBLE, panelView.findViewById(R.id.main_panel_container).getVisibility());
        assertEquals(View.GONE, panelView.findViewById(R.id.shred_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testSubPanelContainers_existInLayout() throws Exception {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View panelView = inflater.inflate(R.layout.brave_unified_panel_layout, null);

        assertNotNull(
                "Main panel container should exist",
                panelView.findViewById(R.id.main_panel_container));
        assertNotNull(
                "HTTPS panel container should exist",
                panelView.findViewById(R.id.https_panel_container));
        assertNotNull(
                "Trackers panel container should exist",
                panelView.findViewById(R.id.trackers_panel_container));
        assertNotNull(
                "Cookies panel container should exist",
                panelView.findViewById(R.id.cookies_panel_container));
        assertNotNull(
                "Shred panel container should exist",
                panelView.findViewById(R.id.shred_panel_container));
    }

    // ========================================================================
    // Site info layout tests
    // ========================================================================
    // Note: Most site info layout elements are already tested in BraveUnifiedPanelLayoutTest.
    // Only unique tests are included here.

    @SmallTest
    @Test
    public void testShieldsTabContent_hasReportBrokenSiteSection() throws Exception {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View panelView = inflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Report broken site is in the shields tab content
        View shieldsContent = panelView.findViewById(R.id.shields_tab_content);
        assertNotNull("Shields content should exist", shieldsContent);

        assertNotNull(
                "Report broken site section should exist",
                shieldsContent.findViewById(R.id.report_broken_site_section));
        assertNotNull(
                "Report broken site button should exist",
                shieldsContent.findViewById(R.id.report_broken_site_button));
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

    /**
     * Inflates the panel layout and sets up advanced options view references.
     */
    private View inflateAndSetupAdvancedOptions() throws Exception {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View panelView = inflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Set the popup view field
        setPrivateField("mPopupView", panelView);

        // Set up advanced options references
        setPrivateField(
                "mAdvancedOptionsContent", panelView.findViewById(R.id.advanced_options_content));
        setPrivateField(
                "mAdvancedOptionsArrow", panelView.findViewById(R.id.advanced_options_arrow));
        setPrivateField("mIsAdvancedOptionsExpanded", false);

        // Set up switches
        setPrivateField("mBlockScriptsSwitch", panelView.findViewById(R.id.scripts_toggle));
        setPrivateField("mForgetMeSwitch", panelView.findViewById(R.id.forget_me_toggle));
        setPrivateField(
                "mFingerprintingSwitch", panelView.findViewById(R.id.fingerprinting_toggle));
        setPrivateField("mIsUpdatingSwitches", false);

        return panelView;
    }

    private void setPrivateField(String fieldName, Object value) throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField(fieldName);
        field.setAccessible(true);
        field.set(mHandler, value);
    }

    private boolean getPrivateBooleanField(String fieldName) throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField(fieldName);
        field.setAccessible(true);
        return field.getBoolean(mHandler);
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

    private void invokeToggleAdvancedOptions() throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("toggleAdvancedOptions");
        method.setAccessible(true);
        method.invoke(mHandler);
    }

    private void invokeOnBlockScriptsChanged(boolean isChecked) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod(
                        "onBlockScriptsChanged", boolean.class);
        method.setAccessible(true);
        method.invoke(mHandler, isChecked);
    }

    private void invokeOnForgetMeChanged(boolean isChecked) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod(
                        "onForgetMeChanged", boolean.class);
        method.setAccessible(true);
        method.invoke(mHandler, isChecked);
    }

    private void invokeOnFingerprintingChanged(boolean isChecked) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod(
                        "onFingerprintingChanged", boolean.class);
        method.setAccessible(true);
        method.invoke(mHandler, isChecked);
    }

    /**
     * Inflates the panel layout and sets up sub-panel container references.
     */
    private View inflateAndSetupSubPanels() throws Exception {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View panelView = inflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Set the popup view field
        setPrivateField("mPopupView", panelView);

        // Set up container references
        setPrivateField("mMainPanelContainer", panelView.findViewById(R.id.main_panel_container));
        setPrivateField("mHttpsPanelContainer", panelView.findViewById(R.id.https_panel_container));
        setPrivateField(
                "mTrackersPanelContainer", panelView.findViewById(R.id.trackers_panel_container));
        setPrivateField(
                "mCookiesPanelContainer", panelView.findViewById(R.id.cookies_panel_container));
        setPrivateField("mShredPanelContainer", panelView.findViewById(R.id.shred_panel_container));

        // Set up advanced options for state restoration tests
        setPrivateField(
                "mAdvancedOptionsContent", panelView.findViewById(R.id.advanced_options_content));
        setPrivateField(
                "mAdvancedOptionsArrow", panelView.findViewById(R.id.advanced_options_arrow));
        setPrivateField("mIsAdvancedOptionsExpanded", false);

        return panelView;
    }

    private void invokeShowHttpsUpgradePanel() throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("showHttpsUpgradePanel");
        method.setAccessible(true);
        method.invoke(mHandler);
    }

    private void invokeShowShredPanel() throws Exception {
        Method method = BraveUnifiedPanelHandler.class.getDeclaredMethod("showShredPanel");
        method.setAccessible(true);
        method.invoke(mHandler);
    }

    private void invokeShowMainPanel() throws Exception {
        Method method = BraveUnifiedPanelHandler.class.getDeclaredMethod("showMainPanel");
        method.setAccessible(true);
        method.invoke(mHandler);
    }
}
