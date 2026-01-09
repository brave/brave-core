/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;

/**
 * Robolectric tests for the Unified Panel layout XML files.
 *
 * These tests verify that:
 * - All panel layouts inflate successfully without errors
 * - Critical view IDs required by BraveUnifiedPanelHandler are present
 * - The layout hierarchy is correctly structured
 */
@RunWith(BaseRobolectricTestRunner.class)
public final class BraveUnifiedPanelLayoutTest {
    private Activity mActivity;
    private LayoutInflater mInflater;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mInflater = LayoutInflater.from(mActivity);
    }

    @SmallTest
    @Test
    public void testMainPanelLayout_inflatesSuccessfully() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);
        assertNotNull("Main panel layout should inflate successfully", panelView);
        assertTrue(
                "Root view should be a FrameLayout", panelView instanceof FrameLayout);
    }

    @SmallTest
    @Test
    public void testMainPanelLayout_hasRequiredContainers() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Verify main container structure
        assertNotNull(
                "Main panel container must exist",
                panelView.findViewById(R.id.main_panel_container));
        assertTrue(
                "Main panel container should be a ScrollView",
                panelView.findViewById(R.id.main_panel_container) instanceof ScrollView);

        // Verify sub-panel containers exist
        assertNotNull(
                "HTTPS panel container must exist",
                panelView.findViewById(R.id.https_panel_container));
        assertNotNull(
                "Trackers panel container must exist",
                panelView.findViewById(R.id.trackers_panel_container));
        assertNotNull(
                "Cookies panel container must exist",
                panelView.findViewById(R.id.cookies_panel_container));
        assertNotNull(
                "Permissions panel container must exist",
                panelView.findViewById(R.id.permissions_panel_container));
        assertNotNull(
                "Shred panel container must exist",
                panelView.findViewById(R.id.shred_panel_container));
    }

    @SmallTest
    @Test
    public void testMainPanelLayout_hasTabContent() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Verify tab content areas exist (these are includes in the main layout)
        assertNotNull(
                "Shields tab content must exist",
                panelView.findViewById(R.id.shields_tab_content));
        assertNotNull(
                "Site settings tab content must exist",
                panelView.findViewById(R.id.site_settings_tab_content));
    }

    @SmallTest
    @Test
    public void testTabsLayout_hasRequiredTabElements() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Verify tab buttons exist (from included brave_unified_tabs_layout.xml)
        View shieldsTabButton = panelView.findViewById(R.id.shields_tab_button);
        View siteSettingsTabButton = panelView.findViewById(R.id.site_settings_tab_button);

        assertNotNull("Shields tab button must exist", shieldsTabButton);
        assertNotNull("Site settings tab button must exist", siteSettingsTabButton);

        assertTrue(
                "Tab buttons should be LinearLayouts",
                shieldsTabButton instanceof LinearLayout
                        && siteSettingsTabButton instanceof LinearLayout);

        // Verify tab icons and text
        assertNotNull(
                "Shields tab icon must exist", panelView.findViewById(R.id.shields_tab_icon));
        assertNotNull(
                "Shields tab text must exist", panelView.findViewById(R.id.shields_tab_text));
        assertNotNull(
                "Site settings tab icon must exist",
                panelView.findViewById(R.id.site_settings_tab_icon));
        assertNotNull(
                "Site settings tab text must exist",
                panelView.findViewById(R.id.site_settings_tab_text));

        // Verify tab indicators
        assertNotNull(
                "Shields tab indicator must exist",
                panelView.findViewById(R.id.shields_tab_indicator));
        assertNotNull(
                "Site settings tab indicator must exist",
                panelView.findViewById(R.id.site_settings_tab_indicator));
    }

    @SmallTest
    @Test
    public void testTabsLayout_tabButtonsAreClickable() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        View shieldsTabButton = panelView.findViewById(R.id.shields_tab_button);
        View siteSettingsTabButton = panelView.findViewById(R.id.site_settings_tab_button);

        assertTrue("Shields tab button should be clickable", shieldsTabButton.isClickable());
        assertTrue(
                "Site settings tab button should be clickable",
                siteSettingsTabButton.isClickable());
    }

    @SmallTest
    @Test
    public void testShieldsTabContent_hasRequiredElements() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // These are critical elements used by BraveUnifiedPanelHandler for shields display
        assertNotNull(
                "Shield icon must exist for shields status display",
                panelView.findViewById(R.id.shield_icon_unified));
        assertNotNull(
                "Shields toggle switch must exist",
                panelView.findViewById(R.id.shields_toggle_switch));
        assertNotNull(
                "Blocked items container must exist",
                panelView.findViewById(R.id.blocked_items_container));
    }

    @SmallTest
    @Test
    public void testShieldsTabContent_hasAdvancedOptions() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Advanced options section
        assertNotNull(
                "Advanced options button must exist",
                panelView.findViewById(R.id.advanced_options_button));
        assertNotNull(
                "Advanced options content must exist",
                panelView.findViewById(R.id.advanced_options_content));
        assertNotNull(
                "Advanced options arrow must exist",
                panelView.findViewById(R.id.advanced_options_arrow));
    }

    @SmallTest
    @Test
    public void testSiteSettingsTabContent_hasRequiredElements() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Site settings tab content
        assertNotNull(
                "Site name text must exist",
                panelView.findViewById(R.id.site_name_text));
        assertNotNull(
                "Permissions summary must exist",
                panelView.findViewById(R.id.permissions_summary));
    }

    @SmallTest
    @Test
    public void testSiteSettingsTabContent_hasNavigationElements() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Navigation items in site settings
        assertNotNull(
                "Dangerous site item must exist",
                panelView.findViewById(R.id.dangerous_site_item));
        assertNotNull(
                "Permissions item must exist",
                panelView.findViewById(R.id.permissions_item));
    }

    @SmallTest
    @Test
    public void testSubPanelContainers_defaultVisibility() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Main panel should be visible by default
        assertEquals(
                "Main panel container should be visible",
                View.VISIBLE,
                panelView.findViewById(R.id.main_panel_container).getVisibility());

        // Sub-panels should be gone by default
        assertEquals(
                "HTTPS panel should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.https_panel_container).getVisibility());
        assertEquals(
                "Trackers panel should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.trackers_panel_container).getVisibility());
        assertEquals(
                "Cookies panel should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.cookies_panel_container).getVisibility());
        assertEquals(
                "Permissions panel should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.permissions_panel_container).getVisibility());
        assertEquals(
                "Shred panel should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.shred_panel_container).getVisibility());
    }

    @SmallTest
    @Test
    public void testTabContentAreas_defaultVisibility() {
        View panelView = mInflater.inflate(R.layout.brave_unified_panel_layout, null);

        // Shields tab should be visible by default
        assertEquals(
                "Shields tab content should be visible by default",
                View.VISIBLE,
                panelView.findViewById(R.id.shields_tab_content).getVisibility());

        // Site settings tab should be gone by default
        assertEquals(
                "Site settings tab content should be gone by default",
                View.GONE,
                panelView.findViewById(R.id.site_settings_tab_content).getVisibility());
    }

    @SmallTest
    @Test
    public void testPermissionsPanel_inflatesSuccessfully() {
        // Test the permissions panel layout directly
        View permissionsPanel =
                mInflater.inflate(R.layout.brave_permissions_panel, null);
        assertNotNull("Permissions panel layout should inflate successfully", permissionsPanel);

        // Verify key elements exist
        assertNotNull(
                "Permissions list must exist",
                permissionsPanel.findViewById(R.id.permissions_list));
        assertNotNull(
                "Reset permissions button must exist",
                permissionsPanel.findViewById(R.id.reset_permissions_button));
    }

    @SmallTest
    @Test
    public void testHttpsUpgradePanel_inflatesSuccessfully() {
        View httpsPanel = mInflater.inflate(R.layout.brave_https_upgrade_panel, null);
        assertNotNull("HTTPS upgrade panel layout should inflate successfully", httpsPanel);
    }

    @SmallTest
    @Test
    public void testTrackersAdsPanel_inflatesSuccessfully() {
        View trackersPanel = mInflater.inflate(R.layout.brave_trackers_ads_panel, null);
        assertNotNull("Trackers/ads panel layout should inflate successfully", trackersPanel);

        // Verify key elements exist
        assertNotNull(
                "Trackers back button must exist",
                trackersPanel.findViewById(R.id.trackers_back_button));
        assertNotNull(
                "Trackers radio group must exist",
                trackersPanel.findViewById(R.id.trackers_ads_radio_group));
    }

    @SmallTest
    @Test
    public void testCookiesPanel_inflatesSuccessfully() {
        View cookiesPanel = mInflater.inflate(R.layout.brave_cookies_panel, null);
        assertNotNull("Cookies panel layout should inflate successfully", cookiesPanel);
    }

    @SmallTest
    @Test
    public void testShredDataPanel_inflatesSuccessfully() {
        View shredPanel = mInflater.inflate(R.layout.brave_shred_data_panel, null);
        assertNotNull("Shred data panel layout should inflate successfully", shredPanel);
    }
}
