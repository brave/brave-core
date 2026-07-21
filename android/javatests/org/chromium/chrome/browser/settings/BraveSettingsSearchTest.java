/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.action.ViewActions.closeSoftKeyboard;
import static androidx.test.espresso.action.ViewActions.typeText;
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDescendantOfA;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.Matchers.equalToIgnoringCase;

import static org.chromium.ui.test.util.ViewUtils.onViewWaiting;

import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;

import androidx.test.filters.SmallTest;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
import org.chromium.brave.browser.customize_menu.MenuItemData;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.R;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Tests for the global "Search in Settings" feature.
 *
 * <p>Covers three categories:
 *
 * <ol>
 *   <li>Brave-specific settings that must be discoverable via search.
 *   <li>Chrome settings removed by Brave that must NOT appear in search results.
 *   <li>Brave settings that must open the correct screen when tapped from search.
 * </ol>
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DoNotBatch(reason = "Each test launches a fresh SettingsActivity.")
public class BraveSettingsSearchTest {

    @Rule
    // null fragment class so EXTRA_SHOW_FRAGMENT is unset, which is required for
    // SettingsActivity to call createSearchCoordinator() and initialize the search UI.
    public final SettingsActivityTestRule<?> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(null);

    // ---------------------------------------------------------------------------
    // Category 1 — Brave settings that must be findable via search
    // ---------------------------------------------------------------------------

    /**
     * Verifies that key Brave-specific settings entries appear in the global Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(BraveFeatureList.BRAVE_PLAYLIST)
    public void testBraveMainSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Features section

        typeIntoSearch("Brave Shields");
        assertSearchResult("Brave Shields & privacy");

        clearAndTypeIntoSearch("Brave News");
        assertSearchResult("Brave News");

        clearAndTypeIntoSearch("Brave Wallet");
        assertSearchResult("Brave Wallet");

        clearAndTypeIntoSearch("Firewall");
        assertSearchResult("Brave Firewall + VPN");

        clearAndTypeIntoSearch("Leo AI");
        assertSearchResult("Leo AI");

        clearAndTypeIntoSearch("Brave Playlist");
        assertSearchResult("Brave Playlist");

        // General section

        clearAndTypeIntoSearch("Search engines");
        assertSearchResult("Search engines");

        // Note: result appears under "Advanced" instead of "General"; see
        // https://github.com/brave/brave-browser/issues/57198
        clearAndTypeIntoSearch("Homepage");
        assertSearchResult("Homepage");

        clearAndTypeIntoSearch("Home screen widget");
        assertSearchResult("Home screen widget");

        // Disabled — result appears under "Display" instead of "General"; see
        // https://github.com/brave/brave-browser/issues/57198
        // clearAndTypeIntoSearch("Sync");
        // assertSearchResult("Sync");

        clearAndTypeIntoSearch("Privacy Report");
        assertSearchResult("Privacy Report");

        // Disabled — result appears under "Advanced" instead of "General"; see
        // https://github.com/brave/brave-browser/issues/57198
        // clearAndTypeIntoSearch("Notifications");
        // assertSearchResult("Notifications");

        clearAndTypeIntoSearch("Site settings");
        assertSearchResult("Site settings");

        // Disabled — expected in "General" but found under "Support"; see
        // https://github.com/brave/brave-browser/issues/57194
        // clearAndTypeIntoSearch("Downloads");
        // assertOneOfSearchResultsIs("Downloads", "General");

        // Disabled — this switch now lives on the Tabs and tab groups screen; see
        // https://github.com/brave/brave-browser/issues/57179
        // clearAndTypeIntoSearch("Closing all tabs closes Brave");
        // assertSearchResult("Closing all tabs closes Brave");

        clearAndTypeIntoSearch("Open external links in Brave");
        assertSearchResult("Open external links in Brave");

        clearAndTypeIntoSearch("Brave Origin");
        assertSearchResult("Brave Origin");

        // Display section

        clearAndTypeIntoSearch("Tabs and tab groups");
        assertSearchResult("Tabs and tab groups");

        clearAndTypeIntoSearch("Media");
        assertSearchResult("Media");

        clearAndTypeIntoSearch("Appearance");
        assertSearchResult("Appearance");

        clearAndTypeIntoSearch("New Tab Page");
        assertSearchResult("New Tab Page");

        clearAndTypeIntoSearch("Accessibility");
        assertSearchResult("Accessibility");

        clearAndTypeIntoSearch("Languages");
        assertSearchResult("Languages");

        // Passwords and autofill section

        clearAndTypeIntoSearch("Brave Password Manager");
        assertSearchResult("Brave Password Manager");

        // Disabled — see https://github.com/brave/brave-browser/issues/57195
        // clearAndTypeIntoSearch("Autofill services");
        // assertSearchResult("Autofill services");

        clearAndTypeIntoSearch("Payment methods");
        assertSearchResult("Payment methods");

        clearAndTypeIntoSearch("Addresses and more");
        assertSearchResult("Addresses and more");

        // Disabled — see https://github.com/brave/brave-browser/issues/57195
        // clearAndTypeIntoSearch("Autofill in private tabs");
        // assertSearchResult("Autofill in private tabs");

        // Support section

        // Disabled — see https://github.com/brave/brave-browser/issues/57196
        // clearAndTypeIntoSearch("Rate Brave");
        // assertSearchResult("Rate Brave");

        // About section

        clearAndTypeIntoSearch("Developer options");
        assertSearchResult("Developer options");

        clearAndTypeIntoSearch("About Brave");
        assertSearchResult("About Brave");
    }

    /**
     * Verifies the counterpart of {@link #testBraveMainSettingsAreSearchable}: when the Playlist
     * feature is disabled, the "Brave Playlist" entry is not indexed and cannot be found in search.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableFeatures(BraveFeatureList.BRAVE_PLAYLIST)
    public void testBravePlaylistNotSearchable_FeatureDisabled() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Brave Playlist");
        assertSearchResultEmpty();
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the Brave Shields & privacy
     * Settings search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveShieldsAndPrivacySettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Disabled — see https://github.com/brave/brave-browser/issues/57186
        // typeIntoSearch("Safe Browsing");
        // assertSearchResult("Safe Browsing");

        // Disabled — see https://github.com/brave/brave-browser/issues/57186
        // Brave Shields & privacy => Lock Private tabs when you leave Brave
        // clearAndTypeIntoSearch("Lock Private tabs");
        // assertSearchResult("Lock Private tabs");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Brave News` Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveNewsSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Disabled — see https://github.com/brave/brave-browser/issues/57187
        // typeIntoSearch("Show Brave News");
        // assertSearchResult("Show Brave News");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Brave Wallet` Settings
     * search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveWalletSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Default Ethereum wallet");
        assertSearchResult("Default Ethereum wallet");

        clearAndTypeIntoSearch("Default Ethereum wallet");
        assertSearchResult("Default Ethereum wallet");

        clearAndTypeIntoSearch("Automatically lock Brave Wallet");
        assertSearchResult("Automatically lock Brave Wallet");

        clearAndTypeIntoSearch("Networks");
        assertSearchResult("Networks");

        clearAndTypeIntoSearch("Display Web3 notifications");
        assertSearchResult("Display Web3 notifications");

        clearAndTypeIntoSearch("Clear transaction & nonce info");
        assertSearchResult("Clear transaction & nonce info");

        clearAndTypeIntoSearch("Enable NFT discovery");
        assertSearchResult("Enable NFT discovery");

        // Disabled — see https://github.com/brave/brave-browser/issues/57188
        // clearAndTypeIntoSearch("Automatically add NTFs");
        // assertSearchResult("Automatically add NTFs you own to the Wallet using third party
        // APIs");

        clearAndTypeIntoSearch("Reset and clear wallet data");
        assertSearchResult("Reset and clear wallet data");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Brave Firewall + VPN`
     * Settings search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveFirewallVpnSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Enabled");
        assertSearchResult("Enabled", "Brave Firewall + VPN");

        clearAndTypeIntoSearch("Status");
        assertSearchResult("Status");

        clearAndTypeIntoSearch("Expires");
        assertSearchResult("Expires");

        clearAndTypeIntoSearch("Manage subscription");
        assertSearchResult("Manage subscription");

        clearAndTypeIntoSearch("Auto re-connect VPN");
        assertSearchResult("Auto re-connect VPN");

        clearAndTypeIntoSearch("Split tunneling");
        assertSearchResult("Split tunneling");

        clearAndTypeIntoSearch("Reset Configuration");
        assertSearchResult("Reset Configuration");

        clearAndTypeIntoSearch("Contact Technical Support");
        assertSearchResult("Contact Technical Support");

        clearAndTypeIntoSearch("VPN support");
        assertSearchResult("VPN support");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Leo AI` Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveLeoAiSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Show autocomplete suggestions");
        assertSearchResult("Show autocomplete suggestions in address bar");

        clearAndTypeIntoSearch("Show in quick search engines bar");
        assertSearchResult("Show in quick search engines bar");

        clearAndTypeIntoSearch("Store my conversation history");
        assertSearchResult("Store my conversation history");

        clearAndTypeIntoSearch("Default model");
        assertSearchResult("Default model for new conversations");

        // Disabled — go_premium in brave_leo_preferences.xml is marked
        // app:isPreferenceVisible="false", so it is excluded from the search index.
        // See https://github.com/brave/brave-browser/issues/57193
        // clearAndTypeIntoSearch("Go Premium");
        // assertSearchResult("Go Premium");

        clearAndTypeIntoSearch("Delete all Leo AI");
        assertSearchResult("Delete all Leo AI conversation data");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Search engines` Settings
     * search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSearchEnginesSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Standard Tab");
        assertSearchResult("Standard Tab");

        clearAndTypeIntoSearch("Private Tab");
        assertSearchResult("Private Tab");

        clearAndTypeIntoSearch("Quick-Search");
        assertSearchResult("Quick-Search Engines");

        // Sub-section, custom layout — see
        // https://github.com/brave/brave-browser/issues/57189
        // clearAndTypeIntoSearch("Show quick search bar");
        // assertSearchResult("Show quick search bar");

        clearAndTypeIntoSearch("Show browser suggestions");
        assertSearchResult("Show browser suggestions");

        clearAndTypeIntoSearch("Show search suggestions");
        assertSearchResult("Show search suggestions");

        clearAndTypeIntoSearch("Web Discovery Project");
        assertSearchResult("Web Discovery Project");

        // custom layout — see https://github.com/brave/brave-browser/issues/57189
        // clearAndTypeIntoSearch("Show quick search bar");
        // assertSearchResult("Show quick search bar");
        // clearAndTypeIntoSearch("Add a custom search engine");
        // assertSearchResult("Add a custom search engine");
    }

    // There are no indexed setting to search at Homepage item

    // Sync - no indexed setting

    /**
     * Verifies that key Brave-specific settings entries appear in the `Privacy Report` Settings
     * search results.
     */
    // Disabled: BraveStatsPreferences has no search index provider.
    // TODO(https://github.com/brave/brave-browser/issues/57183)
    // @Test
    // @SmallTest
    // @Feature({"Preferences"})
    // public void testPrivacyReportSettingsAreSearchable() {
    //     mSettingsActivityTestRule.startSettingsActivity();

    //     typeIntoSearch("Privacy Report");
    //     assertSearchResult("Privacy Report", "Privacy Report");

    //     clearAndTypeIntoSearch("Privacy Report Notification");
    //     assertSearchResult("Privacy Report Notification");

    //     clearAndTypeIntoSearch("Clear all privacy reports data");
    //     assertSearchResult("Clear all privacy reports data");
    // }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Site settings` Settings
     * search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSiteSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("All sites");
        assertSearchResult("All sites");

        // Permission

        clearAndTypeIntoSearch("Location");
        assertSearchResult("Location");

        clearAndTypeIntoSearch("Camera");
        assertSearchResult("Camera");

        clearAndTypeIntoSearch("Microphone");
        assertSearchResult("Microphone");

        clearAndTypeIntoSearch("notif");
        // There are notifications both at
        //  - Advanced
        //  - Brave Wallet
        //  - Site settings
        assertOneOfSearchResultsIs("Notifications", "Site settings");

        clearAndTypeIntoSearch("Motion sensors");
        assertSearchResult("Motion sensors");

        clearAndTypeIntoSearch("NFC devices");
        assertSearchResult("NFC devices");

        clearAndTypeIntoSearch("USB");
        assertSearchResult("USB");

        clearAndTypeIntoSearch("Serial port");
        assertSearchResult("Serial port");

        clearAndTypeIntoSearch("File editing");
        assertSearchResult("File editing");

        clearAndTypeIntoSearch("Clipboard");
        assertSearchResult("Clipboard");

        clearAndTypeIntoSearch("Virtual reality");
        assertSearchResult("Virtual reality");

        clearAndTypeIntoSearch("Augmented reality");
        assertSearchResult("Augmented reality");

        clearAndTypeIntoSearch("Local network");
        assertSearchResult("Local network");

        clearAndTypeIntoSearch("Apps on device");
        assertSearchResult("Apps on device");

        // Content

        clearAndTypeIntoSearch("Third-party cookies");
        assertSearchResult("Third-party cookies");

        clearAndTypeIntoSearch("JavaScript");
        assertSearchResult("JavaScript", "Site settings");

        clearAndTypeIntoSearch("Pop-ups");
        assertSearchResult("Pop-ups and redirects");

        clearAndTypeIntoSearch("Sound");
        assertSearchResult("Sound", "Site settings");

        clearAndTypeIntoSearch("Protected content");
        assertSearchResult("Protected content");

        clearAndTypeIntoSearch("On-device site data");
        assertSearchResult("On-device site data");

        clearAndTypeIntoSearch("Desktop site");
        assertSearchResult("Desktop site");

        clearAndTypeIntoSearch("Automatic downloads");
        assertSearchResult("Automatic downloads");

        clearAndTypeIntoSearch("JavaScript opti");
        assertSearchResult("JavaScript optimization & security");

        clearAndTypeIntoSearch("Saved zoom");
        // There is `Saved zoom` both at:
        //  - Accessibility
        //  - Site settings
        assertOneOfSearchResultsIs("Saved zoom for sites", "Site settings");

        clearAndTypeIntoSearch("Data stored");
        assertSearchResult("Data stored");

        clearAndTypeIntoSearch("Autoplay");
        assertSearchResult("Autoplay");

        clearAndTypeIntoSearch("Google Sign-in");
        assertSearchResult("Google Sign-in");

        clearAndTypeIntoSearch("Ethereum");
        assertSearchResult("Ethereum");

        clearAndTypeIntoSearch("Solana");
        assertSearchResult("Solana");

        clearAndTypeIntoSearch("remove perm");
        assertSearchResult("Automatically remove permissions");
    }

    // Disabled — see https://github.com/brave/brave-browser/issues/57194
    // /**
    //  * Verifies that key Brave-specific settings entries appear in the `Downloads` Settings
    //  * search results.
    //  */
    // @Test
    // @SmallTest
    // @Feature({"Preferences"})
    // public void testDownloadsSettingsAreSearchable() {
    //     mSettingsActivityTestRule.startSettingsActivity();

    //     typeIntoSearch("Download location");
    //     assertSearchResult("Download location");

    //     clearAndTypeIntoSearch("Ask where to save files");
    //     assertSearchResult("Ask where to save files");

    //     clearAndTypeIntoSearch("Automatically open when possible");
    //     assertSearchResult("Automatically open when possible");

    //     clearAndTypeIntoSearch("Show download progress notifications");
    //     assertSearchResult("Show download progress notifications");

    //     clearAndTypeIntoSearch("Parallel Downloading");
    //     assertSearchResult("Parallel Downloading");
    // }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Brave Origin` Settings
     * search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testBraveOriginSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();
        // TODO: needs to be enabled to test
    }

    // Display section

    /**
     * Verifies that key Brave-specific settings entries appear in the `Tabs and tab groups`
     * Settings search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testTabsAndTabGroupsSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Tabs
        typeIntoSearch("Move to inactive section");
        assertSearchResult("Move to inactive section");

        clearAndTypeIntoSearch("Automatically open tab groups");
        assertSearchResult("Automatically open tab groups from other devices");

        // Inner of `Move to inactive section`
        clearAndTypeIntoSearch("inactive");
        assertOneOfSearchResultsIs("Automatically close inactive items", "Tabs and tab groups");
        assertOneOfSearchResultsIs("Move to inactive section", "Tabs and tab groups");
        assertOneOfSearchResultsIs(
                "Choose when tabs and tab groups are automatically moved to the inactive section.",
                "Tabs and tab groups");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Tabs and tab groups`
     * Settings search results when Brave Android Tab Groups feature is enabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testTabsAndTabGroupsSettingsAreSearchable_FeatureEnabled() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Groups
        // Disabled: "Enable tab groups", "Tab groups bar" and "Open links in current tab group"
        // are not present in the settings search index even with the feature enabled (verified on
        // device), and the "Tabs and tab groups" screen opened from a search result is incomplete.
        // TODO(https://github.com/brave/brave-browser/issues/57179)
        // clearAndTypeIntoSearch("Enable tab groups");
        // assertSearchResult("Enable tab groups");

        typeIntoSearch("Automatically open tab groups from other devices");
        assertSearchResult("Automatically open tab groups from other devices");

        // Disabled — see https://github.com/brave/brave-browser/issues/57179
        // clearAndTypeIntoSearch("Tab groups bar");
        // assertSearchResult("Tab groups bar");

        // Disabled — see https://github.com/brave/brave-browser/issues/57179
        // clearAndTypeIntoSearch("Open links in current tab group");
        // assertSearchResult("Open links in current tab group");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the `Tabs and tab groups`
     * Settings search results when Brave Android Tab Groups feature is enabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS)
    public void testTabsAndTabGroupsSettingsAreSearchable_FeatureDisabled() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Enable tab groups");
        assertSearchResultEmpty();

        // Disabled — found even when feature disabled; see
        // https://github.com/brave/brave-browser/issues/57179
        // typeIntoSearch("Automatically open tab groups from other devices");
        // assertSearchResultEmpty();

        clearAndTypeIntoSearch("Tab groups bar");
        assertSearchResultEmpty();

        // Disabled — found even when feature disabled; see
        // https://github.com/brave/brave-browser/issues/57179
        // clearAndTypeIntoSearch("Open links in current tab group");
        // assertSearchResultEmpty();
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the Media Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testMediaSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // General
        typeIntoSearch("Widevine DRM");
        assertSearchResult("Widevine DRM");

        clearAndTypeIntoSearch("Background play");
        assertSearchResult("Background play");

        // YouTube
        clearAndTypeIntoSearch("Open YouTube links in Brave");
        assertSearchResult("Open YouTube links in Brave");

        clearAndTypeIntoSearch("Block YouTube Shorts");
        assertSearchResult("Block YouTube Shorts");

        clearAndTypeIntoSearch("Block YouTube Playables");
        assertSearchResult("Block YouTube Playables");

        clearAndTypeIntoSearch("Block YouTube recommended content");
        assertSearchResult("Block YouTube recommended content");

        clearAndTypeIntoSearch("Block YouTube distracting elements");
        assertSearchResult("Block YouTube distracting elements");

        clearAndTypeIntoSearch("Block YouTube auto-dubbed videos");
        assertSearchResult("Block YouTube auto-dubbed videos");

        clearAndTypeIntoSearch("Block YouTube members-only videos");
        assertSearchResult("Block YouTube members-only videos");
    }

    /**
     * Verifies that the "Background play" Media setting is not searchable when the Brave background
     * video playback feature is disabled.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableFeatures(BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK)
    public void testMediaSettingsAreSearchable_BackgroundPlayFeatureDisabled() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Background play");
        assertSearchResultEmpty();
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the Appearance Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAppearanceSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Theme");
        assertSearchResult("Theme");

        clearAndTypeIntoSearch("Customize menu");
        assertSearchResult("Customize menu");

        clearAndTypeIntoSearch("Toolbar shortcut");
        // Seeing two results, but Chromuim also has two
        assertOneOfSearchResultsIs("Toolbar shortcut", "Appearance");

        clearAndTypeIntoSearch("Address bar");
        assertSearchResult("Address bar");

        clearAndTypeIntoSearch("bottom navigation");
        assertSearchResult("Bottom navigation toolbar");

        clearAndTypeIntoSearch("Brave Rewards icon");
        assertSearchResult("Brave Rewards icon");

        clearAndTypeIntoSearch("Brave Ads");
        assertSearchResult("Brave Ads");

        clearAndTypeIntoSearch("Night");
        assertSearchResult("Night Mode");

        clearAndTypeIntoSearch("Only open links in current tab group");
        assertSearchResult("Only open links in current tab group");

        clearAndTypeIntoSearch("Multiple windows");
        assertSearchResult("Multiple windows");

        clearAndTypeIntoSearch("Show undo button when tabs are closed");
        assertSearchResult("Show undo button when tabs are closed");
    }

    // Disabled: BackgroundImagesPreferences has no search index provider.
    // TODO(https://github.com/brave/brave-browser/issues/57182)
    // /**
    //  * Verifies that key Brave-specific settings entries appear in the `New tab page` Settings
    //  * search results.
    //  */
    // @Test
    // @SmallTest
    // @Feature({"Preferences"})
    // public void testNewTabPageSettingsAreSearchable() {
    //     mSettingsActivityTestRule.startSettingsActivity();

    //     // Background images
    //     typeIntoSearch("Show Background Images");
    //     assertSearchResult("Show Background Images");

    //     clearAndTypeIntoSearch("Show New Tab Page Ads");
    //     assertSearchResult("Show New Tab Page Ads");

    //     clearAndTypeIntoSearch("Learn more about new tab page ads");
    //     assertSearchResult("Learn more about new tab page ads");

    //     // Widgets
    //     clearAndTypeIntoSearch("Show Top Sites");
    //     assertSearchResult("Show Top Sites");

    //     clearAndTypeIntoSearch("Show Brave Stats");
    //     assertSearchResult("Show Brave Stats");
    // }

    /**
     * Verifies that key Brave-specific settings entries appear in the Accessibility Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAccessibilitySettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Default zoom");
        assertSearchResult("Default zoom");

        clearAndTypeIntoSearch("Saved zoom for sites");
        assertOneOfSearchResultsIs("Saved zoom for sites", "Accessibility");

        clearAndTypeIntoSearch("Show zoom option in main menu");
        assertSearchResult("Show zoom option in main menu");

        clearAndTypeIntoSearch("Force enable zoom");
        assertSearchResult("Force enable zoom");

        clearAndTypeIntoSearch("Simplified view for web pages");
        assertSearchResult("Simplified view for web pages");

        clearAndTypeIntoSearch("Navigate pages");
        assertSearchResult("Navigate pages with a text cursor when a keyboard is attached");

        clearAndTypeIntoSearch("Captions");
        assertSearchResult("Captions");

        clearAndTypeIntoSearch("Swipe between pages using a touchpad");
        assertSearchResult("Swipe between pages using a touchpad");

        // Disabled — see https://github.com/brave/brave-browser/issues/57190
        // clearAndTypeIntoSearch("Pull to refresh");
        // assertSearchResult("Pull to refresh");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in the Languages Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testLanguagesSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Disabled — see https://github.com/brave/brave-browser/issues/57191
        // typeIntoSearch("Current device language");
        // assertSearchResult("Current device language");

        typeIntoSearch("Use Brave Translate");
        assertSearchResult("Use Brave Translate");
    }

    // Disabled — see https://github.com/brave/brave-browser/issues/57197
    // /**
    //  * Verifies that key Brave-specific settings entries appear in Brave Password Manager
    //  * Settings search results.
    //  */
    // @Test
    // @SmallTest
    // @Feature({"Preferences"})
    // public void testPasswordManagerSettingsAreSearchable() {
    //     mSettingsActivityTestRule.startSettingsActivity();

    //     typeIntoSearch("Save passwords");
    //     assertSearchResult("Save passwords");

    //     clearAndTypeIntoSearch("Auto Sign-in");
    //     assertSearchResult("Auto Sign-in");

    //     clearAndTypeIntoSearch("Import passwords");
    //     assertSearchResult("Import passwords");

    //     clearAndTypeIntoSearch("Export passwords");
    //     assertSearchResult("Export passwords");
    // }

    // Disabled — see https://github.com/brave/brave-browser/issues/57195
    // /**
    //  * Verifies that key Brave-specific settings entries appear in Autofill Services Settings
    //  * search results.
    //  */
    // @Test
    // @SmallTest
    // @Feature({"Preferences"})
    // public void testAutofillServicesSettingsAreSearchable() {
    //     mSettingsActivityTestRule.startSettingsActivity();

    //     typeIntoSearch("Autofill with Brave");
    //     assertSearchResult("Autofill with Brave");

    //     clearAndTypeIntoSearch("Autofill using another service");
    //     assertSearchResult("Autofill using another service");

    //     clearAndTypeIntoSearch("Autofill in private tabs");
    //     assertSearchResult("Autofill in private tabs");
    // }

    /**
     * Verifies that key Brave-specific settings entries appear in PaymentsMethods Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testPaymentsMethodsSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Save and fill payment methods");
        assertSearchResult("Save and fill payment methods");

        clearAndTypeIntoSearch("Verify it's you to autofill payment methods");
        assertSearchResult("Verify it's you to autofill payment methods");

        clearAndTypeIntoSearch("Save security codes");
        assertSearchResult("Save security codes");

        clearAndTypeIntoSearch("Card benefits");
        assertSearchResult("Card benefits");

        clearAndTypeIntoSearch("Check out faster with autofill");
        assertSearchResult("Check out faster with autofill");

        // Disabled as it is locale-gated. "Add IBAN" is only indexed/shown when
        // shouldShowAddIbanButtonOnSettingsPage() is true, i.e. when the experiment-group
        // country is IBAN-applicable (e.g. AE/EU, but not US) or the user has previously
        // seen an IBAN (prefs::HasSeenIban). The test emulator defaults to a US country with
        // a fresh profile, so the entry is never added to the search index.
        // clearAndTypeIntoSearch("Add IBAN");
        // assertSearchResult("Add IBAN");

        clearAndTypeIntoSearch("Payment apps");
        assertSearchResult("Payment apps");

        // Should not: Loyalty cards
    }

    /**
     * Verifies that key Brave-specific settings entries appear in Addresses and more Settings
     * search results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAddressesAndMoreSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Save and fill addresses");
        assertSearchResult("Save and fill addresses");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in Developer options Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testDeveloperOptionsSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Tracing");
        assertOneOfSearchResultsIs("Tracing", "Developer options");

        clearAndTypeIntoSearch("Tracing mode");
        assertSearchResult("Tracing mode");

        clearAndTypeIntoSearch("QA Preferences");
        assertSearchResult("QA Preferences");

        clearAndTypeIntoSearch("Rewards Debug");
        assertSearchResult("Rewards Debug");

        clearAndTypeIntoSearch("Show Safe Browsing errors");
        assertSearchResult("Show Safe Browsing errors");
    }

    /**
     * Verifies that key Brave-specific settings entries appear in About Brave Settings search
     * results.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAboutBraveSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        typeIntoSearch("Application version");
        assertSearchResult("Application version");

        clearAndTypeIntoSearch("Operating system");
        assertSearchResult("Operating system");

        clearAndTypeIntoSearch("Legal information");
        assertSearchResult("Legal information");
    }

    // ---------------------------------------------------------------------------
    // Category 2 — Chrome settings removed by Brave must NOT appear in search
    // ---------------------------------------------------------------------------
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testRemovedChromeSettingsNotFoundInSearch() {
        mSettingsActivityTestRule.startSettingsActivity();

        // We should not see Chrome-branded settings at all
        typeIntoSearch("Chrome");
        assertSearchResultEmpty();

        // Main settings page of Chrome
        clearAndTypeIntoSearch("Google");
        assertSearchResultDoesNotContain("services");
        assertSearchResultDoesNotContain("Password");

        clearAndTypeIntoSearch("Safety check");
        assertSearchResultEmpty();

        /////////////////////////////////////////////
        // Privacy and security from Chrome we should not have at Brave
        clearAndTypeIntoSearch("Delete browsing data");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Privacy Guide");
        assertSearchResultEmpty();

        // How to ensure "Third-party cookies" are at `Site settings` and not at `Privacy and
        // security`

        clearAndTypeIntoSearch("Preload pages");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Lock Private tabs");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("password was compromised");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("secure connections");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Access payment");
        assertSearchResultEmpty();

        //////////////////////////////////////////
        // Safety check
        clearAndTypeIntoSearch("check passwords in your");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("up to date");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Safe Browsing is on");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Permissions look");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Notification look");
        assertSearchResultEmpty();

        //////////////////////////////////////////
        // Payment methods

        // Disabled — false positive: it finds `Loyalty cards` => `Manage your loyalty cards` in
        // Brave Wallet, but should not be displayed.
        // See https://github.com/brave/brave-browser/issues/57192
        // clearAndTypeIntoSearch("Loyalty cards");
        // assertSearchResultEmpty();

        ///////////////////////////////////////////
        // Autofill settings
        // It's name at Brave is Autofill services
        clearAndTypeIntoSearch("Autofill settings");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("Enhanced autofill");
        assertSearchResultEmpty();

        clearAndTypeIntoSearch("you to autifill sensitive info");
        assertSearchResultEmpty();

        //////////////////////////////////////////
        // Tabs and tag groups
        // All is equal between Brave and Chrome

        //////////////////////////////////////////
        // Home page
        // All is equal between Brave and Chrome

        //////////////////////////////////////////
        // Notifications
        // All is equal between Brave and Chrome

        ///////////////////////////////////////
        // Appearance
        // All is equal between Brave and Chrome, Brave has additional items

        // Appearance => Toolbar shortcut should be shown once
        // `sho`

        ///////////////////////////////////////
        // Accessibility
        // All is equal between Brave and Chrome, Brave has additional item `Pull to refresh`

        ///////////////////////////////////////
        // Site settings
        // All is equal between Brave and Chrome

        ///////////////////////////////////////
        // Downloads
        // All is equal between Brave and Chrome, Brave has additional items

        ///////////////////////////////////////
        // Developer options
        // All is equal between Brave and Chrome, Brave has additional items

        ///////////////////////////////////////
        // About Brave/Chrome => Legal information
        clearAndTypeIntoSearch("Additional Terms of Service");
        assertSearchResultEmpty();

        // Exclude results found through Search in Settings at Chrome browser
        clearAndTypeIntoSearch("Google");
        assertSearchResultDoesNotContain("Help");
        assertSearchResultDoesNotContain("Terms of Service");
        assertSearchResultDoesNotContain("Privacy");
        assertSearchResultDoesNotContain("Translate");
        assertSearchResultDoesNotContain("password was compromised");
        assertSearchResultDoesNotContain("Google Wallet");
    }

    // ---------------------------------------------------------------------------
    // Category 3 — Search results must open the correct Brave screen
    // ---------------------------------------------------------------------------

    /**
     * Verifies that tapping the "Home screen widget" search result triggers the system "pin widget"
     * request rather than opening the "Search engines" screen.
     *
     * <p>The {@code home_screen_widget} entry has no sub-screen; its search result is dispatched by
     * key to {@code MainSettings.openSearchResult()} (redirected to {@code
     * BraveMainPreferencesBase} by bytecode), which calls {@link
     * BraveSearchWidgetUtils#requestPinAppWidget()}. We stub that call so it records invocation
     * instead of popping a real system dialog during the test.
     *
     * <p>Requires the fix from the {@code android_widget_search_results} branch. Without it, the
     * result opens {@code BraveSearchEnginesPreferences} and {@code requestPinAppWidget()} is never
     * called, so this test fails.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testHomeScreenWidgetSearchResultTriggersWidgetPin() {
        AtomicBoolean pinRequested = new AtomicBoolean(false);
        BraveSearchWidgetUtils.setRequestPinAppWidgetForTesting(() -> pinRequested.set(true));
        try {
            mSettingsActivityTestRule.startSettingsActivity();

            typeIntoSearch("Home screen widget");
            assertSearchResult("Home screen widget");

            clickSearchResult("Home screen widget");

            // Tapping the result must trigger the pin-widget request...
            CriteriaHelper.pollInstrumentationThread(
                    pinRequested::get,
                    "requestPinAppWidget() was not called after tapping the Home screen widget"
                            + " search result");

            // ...and must NOT navigate to the Search engines screen.
            assertSearchResultDoesNotContain("Standard Tab");
            assertSearchResultDoesNotContain("Quick-Search Engines");
        } finally {
            BraveSearchWidgetUtils.setRequestPinAppWidgetForTesting(null);
        }
    }

    // Unique, test-only titles for the menu items primed into the Customize menu bundle. Chosen so
    // they cannot collide with any other text on the Customize menu screen.
    private static final String CUSTOMIZE_MENU_ITEM_MAIN_A = "Customize menu test item A";
    private static final String CUSTOMIZE_MENU_ITEM_MAIN_B = "Customize menu test item B";
    private static final String CUSTOMIZE_MENU_ITEM_PAGE_A = "Customize menu test item C";
    private static final String CUSTOMIZE_MENU_ITEM_PAGE_B = "Customize menu test item D";

    /**
     * Verifies that tapping the "Customize menu" search result opens {@link
     * org.chromium.brave.browser.customize_menu.settings.BraveCustomizeMenuPreferenceFragment} with
     * its menu-item switches populated (not an empty screen).
     *
     * <p>Regression test for https://github.com/brave/brave-browser/issues/55088, fixed in
     * f5b06ab28426456e1c0536100e3f8f8176099eeb. The screen is normally populated from a bundle
     * cached the last time the app menu was opened ({@code CustomizeBraveMenu.populateBundle}). No
     * app-menu session runs during this test, so we prime that cached bundle directly. It must be
     * set before the activity starts so the search-index build in {@code BraveMainPreferencesBase}
     * sees a non-null bundle and points the "Customize menu" result at the fragment.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testCustomizeMenuSearchResultsAreClickable() {
        CustomizeBraveMenu.setLastKnownBundleForTesting(createCustomizeMenuTestBundle());
        try {
            mSettingsActivityTestRule.startSettingsActivity();

            typeIntoSearch("Customize menu");
            assertSearchResult("Customize menu");

            clickSearchResult("Customize menu");

            // The screen must be populated with the switch items from the cached bundle rather
            // than being empty (the bug this guards against). Assert every item from both the
            // "Main menu" and "Page actions" sections is shown.
            assertPreferenceTitleDisplayed(CUSTOMIZE_MENU_ITEM_MAIN_A);
            assertPreferenceTitleDisplayed(CUSTOMIZE_MENU_ITEM_MAIN_B);
            assertPreferenceTitleDisplayed(CUSTOMIZE_MENU_ITEM_PAGE_A);
            assertPreferenceTitleDisplayed(CUSTOMIZE_MENU_ITEM_PAGE_B);
        } finally {
            // Avoid leaking the primed bundle into other tests sharing this process.
            CustomizeBraveMenu.setLastKnownBundleForTesting(null);
        }
    }

    /**
     * Builds a Customize menu bundle with a couple of main-menu and page-action items, using real
     * menu resource IDs (required for the fragment's resource-name lookup) but unique, test-only
     * titles so the resulting switch rows can be matched unambiguously.
     */
    private static Bundle createCustomizeMenuTestBundle() {
        ArrayList<MenuItemData> mainMenuItems = new ArrayList<>();
        mainMenuItems.add(
                new MenuItemData(
                        org.chromium.chrome.R.id.new_tab_menu_id,
                        CUSTOMIZE_MENU_ITEM_MAIN_A,
                        /* checked= */ true));
        mainMenuItems.add(
                new MenuItemData(
                        org.chromium.chrome.R.id.downloads_menu_id,
                        CUSTOMIZE_MENU_ITEM_MAIN_B,
                        /* checked= */ false));

        ArrayList<MenuItemData> pageActionItems = new ArrayList<>();
        pageActionItems.add(
                new MenuItemData(
                        org.chromium.chrome.R.id.share_menu_id,
                        CUSTOMIZE_MENU_ITEM_PAGE_A,
                        /* checked= */ true));
        pageActionItems.add(
                new MenuItemData(
                        org.chromium.chrome.R.id.find_in_page_id,
                        CUSTOMIZE_MENU_ITEM_PAGE_B,
                        /* checked= */ false));

        Bundle bundle = new Bundle();
        bundle.putParcelableArrayList(CustomizeBraveMenu.KEY_MAIN_MENU_ITEM_LIST, mainMenuItems);
        bundle.putParcelableArrayList(
                CustomizeBraveMenu.KEY_PAGE_ACTION_ITEM_LIST, pageActionItems);
        return bundle;
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    /**
     * Asserts that no preference-title view with the given text appears in the search results.
     *
     * <p>Waits for the RecyclerView to be visible (i.e. results have loaded), then checks
     * immediately for absence. This avoids false positives where the check races ahead of async
     * result rendering.
     */
    /**
     * Asserts that the search returns no results at all.
     *
     * <p>Unlike {@link #assertSearchResultDoesNotContain}, this does not wait for the RecyclerView
     * (which may not be shown when the result set is empty). Instead it waits for the search UI to
     * be active (search_query_container visible), then confirms no preference-title views exist.
     */
    private void assertSearchResultEmpty() {
        onViewWaiting(withId(R.id.search_query_container)).check(matches(isDisplayed()));
        // Scope to the results (detail) pane: the main-list (header) pane always shows preference
        // titles, so an unscoped check would never observe an empty result set (151 two-pane).
        onView(allOf(withResourceEntryName("title"), inSearchResultsPane())).check(doesNotExist());
    }

    private void assertSearchResultDoesNotContain(String text) {
        // Ensure the results RecyclerView is visible before asserting absence.
        onViewWaiting(allOf(withId(R.id.recycler_view), inSearchResultsPane()))
                .check(matches(isDisplayed()));
        onView(allOf(withText(text), withResourceEntryName("title"), inSearchResultsPane()))
                .check(doesNotExist());
    }

    /**
     * Asserts that a search result with the given text is visible, explicitly excluding the search
     * query EditText which may also contain the same text as an autocomplete suggestion.
     */
    private void assertSearchResult(String text) {
        assertSearchResult(text, /* sectionHeader= */ null);
    }

    /**
     * Asserts that a search result with the given title is visible and, when {@code sectionHeader}
     * is non-null, that the expected section header is also visible on screen. Because section
     * headers and result items are siblings in the RecyclerView (not parent-child), verifying both
     * are present is sufficient to confirm the result lives in the right section.
     */
    private void assertSearchResult(String text, String sectionHeader) {
        if (sectionHeader != null) {
            // Section headers have id=0 (invalid resource ID), so we match by text only, scoped to
            // the results (detail) pane so a same-named main-list item can't satisfy the check.
            onViewWaiting(
                            allOf(
                                    withText(equalToIgnoringCase(sectionHeader)),
                                    inSearchResultsPane()))
                    .check(matches(isDisplayed()));
        }
        // Match specifically the preference title TextView (resource entry name "title").
        // Using withId(R.id.title) fails because the preference library's "title" ID has a
        // different numeric value than the test APK's R.id.title. Matching by entry name
        // works across package boundaries.
        // equalToIgnoringCase: settings search is case-insensitive, so callers may pass
        // lowercase search terms while the indexed title uses different capitalisation.
        onViewWaiting(
                        allOf(
                                withText(equalToIgnoringCase(text)),
                                withResourceEntryName("title"),
                                inSearchResultsPane()))
                .check(matches(isDisplayed()));
    }

    /** Matches a view whose resource entry name equals the given name. */
    /**
     * Asserts that at least one result titled {@code title} appears directly under the section
     * header {@code sectionHeader} in the search results RecyclerView.
     *
     * <p>Unlike {@link #assertSearchResult(String, String)}, this handles the case where multiple
     * results share the same title across different sections (e.g. "Notifications" appears under
     * both "Advanced" and "Site settings"). It traverses RecyclerView children in order, tracking
     * the current section, and verifies the title appears in the correct one.
     */
    private void assertOneOfSearchResultsIs(String title, String sectionHeader) {
        onViewWaiting(allOf(withId(R.id.recycler_view), inSearchResultsPane()))
                .check(
                        (view, e) -> {
                            if (e != null) throw e;
                            ViewGroup rv = (ViewGroup) view;
                            boolean inTargetSection = false;
                            for (int i = 0; i < rv.getChildCount(); i++) {
                                View child = rv.getChildAt(i);
                                if (child instanceof android.widget.TextView) {
                                    // Section headers are direct TextView children of the RV.
                                    String text =
                                            ((android.widget.TextView) child).getText().toString();
                                    inTargetSection = text.equalsIgnoreCase(sectionHeader);
                                } else if (inTargetSection && child instanceof ViewGroup) {
                                    // Preference items are ViewGroup children; find title inside.
                                    android.widget.TextView titleView =
                                            findTitleTextView((ViewGroup) child);
                                    if (titleView != null
                                            && titleView
                                                    .getText()
                                                    .toString()
                                                    .equalsIgnoreCase(title)) {
                                        return; // found
                                    }
                                }
                            }
                            throw new junit.framework.AssertionFailedError(
                                    "'"
                                            + title
                                            + "' not found under section '"
                                            + sectionHeader
                                            + "' in search results");
                        });
    }

    /** Recursively finds the first TextView whose resource entry name is "title". */
    private static android.widget.TextView findTitleTextView(ViewGroup parent) {
        for (int i = 0; i < parent.getChildCount(); i++) {
            View child = parent.getChildAt(i);
            if (child instanceof android.widget.TextView) {
                try {
                    if ("title".equals(child.getResources().getResourceEntryName(child.getId()))) {
                        return (android.widget.TextView) child;
                    }
                } catch (Exception ignored) {
                    // ID not resolvable — not the title view.
                }
            } else if (child instanceof ViewGroup) {
                android.widget.TextView result = findTitleTextView((ViewGroup) child);
                if (result != null) return result;
            }
        }
        return null;
    }

    private static Matcher<View> withResourceEntryName(String name) {
        return new TypeSafeMatcher<View>() {
            @Override
            protected boolean matchesSafely(View view) {
                if (view.getId() == View.NO_ID) return false;
                try {
                    return name.equals(view.getResources().getResourceEntryName(view.getId()));
                } catch (Exception e) {
                    return false;
                }
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("with resource entry name: " + name);
            }
        };
    }

    /**
     * Matches views inside the search-results (detail) pane only. Since Chromium 151, Settings uses
     * a two-pane SlidingPaneLayout: the "preferences_header" pane keeps showing the main settings
     * list while search results render in the "preferences_detail" pane. Both panes contain a
     * RecyclerView with id "recycler_view" and preference "title" views, so result assertions must
     * be scoped here — otherwise they match the main list too and fail as ambiguous (or, for
     * "empty" checks, always see the main-list titles).
     */
    private static Matcher<View> inSearchResultsPane() {
        return isDescendantOfA(withResourceEntryName("preferences_detail"));
    }

    /**
     * Taps a search result with the given title. Matches the preference title TextView (resource
     * entry name "title") so it never targets the search query EditText, which may contain the same
     * text as an autocomplete suggestion.
     */
    private void clickSearchResult(String text) {
        onViewWaiting(
                        allOf(
                                withText(equalToIgnoringCase(text)),
                                withResourceEntryName("title"),
                                inSearchResultsPane()))
                .perform(click());
    }

    /** Asserts that a preference row with the given title is visible on the current screen. */
    private void assertPreferenceTitleDisplayed(String text) {
        onViewWaiting(allOf(withText(equalToIgnoringCase(text)), withResourceEntryName("title")))
                .check(matches(isDisplayed()));
    }

    private void typeIntoSearch(String query) {
        // search_box is the always-visible placeholder; clicking it calls enterSearchState()
        // which hides search_box and makes search_query_container + search_query visible.
        onViewWaiting(withId(R.id.search_box)).perform(click());
        onViewWaiting(withId(R.id.search_query))
                .perform(click(), typeText(query), closeSoftKeyboard());
    }

    private void clearAndTypeIntoSearch(String query) {
        onView(withId(R.id.search_query))
                .perform(
                        click(),
                        androidx.test.espresso.action.ViewActions.clearText(),
                        typeText(query),
                        closeSoftKeyboard());
    }
}
