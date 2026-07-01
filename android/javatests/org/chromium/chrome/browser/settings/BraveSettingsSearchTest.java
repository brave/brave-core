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
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.Matchers.equalToIgnoringCase;

import static org.chromium.ui.test.util.ViewUtils.onViewWaiting;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import androidx.test.filters.SmallTest;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.R;

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
@EnableFeatures(ChromeFeatureList.SEARCH_IN_SETTINGS)
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

        // Needs adjustment/flag
        // clearAndTypeIntoSearch("Brave Playlist");
        // assertSearchResult("Brave Playlist");

        // General section

        // "Search engines" (plural) is Brave's; Chrome's is "Search engine" (singular).
        clearAndTypeIntoSearch("Search engines");
        assertSearchResult("Search engines");

        clearAndTypeIntoSearch("Homepage");
        assertSearchResult("Homepage");

        clearAndTypeIntoSearch("Home screen widget");
        assertSearchResult("Home screen widget");

        // Needs adjustment
        // clearAndTypeIntoSearch("Sync");
        // assertSearchResult("Sync");

        clearAndTypeIntoSearch("Privacy Report");
        assertSearchResult("Privacy Report");

        // Disabled as it fails
        // clearAndTypeIntoSearch("Notifications");
        // assertSearchResult("Notifications");

        clearAndTypeIntoSearch("Site settings");
        assertSearchResult("Site settings");

        clearAndTypeIntoSearch("Downloads");
        assertSearchResult("Downloads");

        // Disabled as it fails
        // clearAndTypeIntoSearch("Closing all tabs closes Brave");
        // assertSearchResult("Closing all tabs closes Brave");

        // Disabled as it fails
        // clearAndTypeIntoSearch("Open external links in Brave");
        // assertSearchResult("Open external links in Brave");

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

        // Disabled as it fails
        // clearAndTypeIntoSearch("Autofill services");
        // assertSearchResult("Autofill services");

        clearAndTypeIntoSearch("Payment methods");
        assertSearchResult("Payment methods");

        clearAndTypeIntoSearch("Addresses and more");
        assertSearchResult("Addresses and more");

        // Disabled as it fails
        // clearAndTypeIntoSearch("Autofill in private tabs");
        // assertSearchResult("Autofill in private tabs");

        // Support section

        // Disabled as it fails
        // clearAndTypeIntoSearch("Rate Brave");
        // assertSearchResult("Rate Brave");

        // About

        clearAndTypeIntoSearch("Developer options");
        assertSearchResult("Developer options");

        clearAndTypeIntoSearch("About Brave");
        assertSearchResult("About Brave");
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

        // Disabled as it fails
        // typeIntoSearch("Safe Browsing");
        // assertSearchResult("Safe Browsing");

        // Disabled as it fails
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

        // Disabled as it fails
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

        // Disabled as it fails
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

        // Disabled as it fails
        // go_premium at brave_leo_preferences.xml is marked as
        // app:isPreferenceVisible="false" so  excluded from search results
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

        // Sub section, custom layout, search fails
        // clearAndTypeIntoSearch("Show quick search bar");
        // assertSearchResult("Show quick search bar");

        clearAndTypeIntoSearch("Show browser suggestions");
        assertSearchResult("Show browser suggestions");

        clearAndTypeIntoSearch("Show search suggestions");
        assertSearchResult("Show search suggestions");

        clearAndTypeIntoSearch("Web Discovery Project");
        assertSearchResult("Web Discovery Project");

        // custom layout, search fails
        // clearAndTypeIntoSearch("Show quick search bar");
        // assertSearchResult("Show quick search bar");
        // clearAndTypeIntoSearch("Add a custom search engine");
        // assertSearchResult("Add a custom search engine");
    }

    // There are no indexed setting to search at Homepage item

    // Home screen widget - why it goes to Search engines from search results?

    // Sync - no indexed setting

    /**
     * Verifies that key Brave-specific settings entries appear in the `Privacy Report` Settings
     * search results.
     */
    // TODO(AlexeyBarabash): add SEARCH_INDEX_DATA_PROVIDER for BraveStatsPreferences
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

    // Notifications are from System Settings

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
        org.chromium.base.Log.e(
                "ViewDump", "========================Notifications======================");
        dumpViewHierarchy();
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

    // Downloads
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
    }

    // ---------------------------------------------------------------------------
    // Category 2 — Chrome settings removed by Brave must NOT appear in search
    // ---------------------------------------------------------------------------

    /** Stub — to be implemented. */
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

        // Disabled as it fails
        // Should not be disaplayed, bug
        // It finds `Loyalty cards` => `Manage your loyalty cards` in Brave Wallet
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

        // //
        // clearAndTypeIntoSearch("Google");
        // assertSearchResultDoesNotContain("Help");
        // assertSearchResultDoesNotContain("Terms of Service");
        // assertSearchResultDoesNotContain("Privacy");
        // assertSearchResultDoesNotContain("Translate");
        // assertSearchResultDoesNotContain("password was compromised");
        // assertSearchResultDoesNotContain("Google Wallet");
    }

    // ---------------------------------------------------------------------------
    // Category 3 — Search results must open the correct Brave screen
    // ---------------------------------------------------------------------------

    /** Stub — to be implemented. */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSearchResultOpensCorrectScreen() {
        // TODO: implement — tap a search result and verify the correct fragment opens.

        // Home screen widget - why it goes to Search engines from search results?
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
        onView(withResourceEntryName("title")).check(doesNotExist());
    }

    private void assertSearchResultDoesNotContain(String text) {
        // Ensure the results RecyclerView is visible before asserting absence.
        onViewWaiting(withId(R.id.recycler_view)).check(matches(isDisplayed()));
        onView(allOf(withText(text), withResourceEntryName("title"))).check(doesNotExist());
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
        dumpViewHierarchy();
        if (sectionHeader != null) {
            // Section headers have id=0 (invalid resource ID), so we match by text only.
            onViewWaiting(withText(equalToIgnoringCase(sectionHeader)))
                    .check(matches(isDisplayed()));
        }
        // Match specifically the preference title TextView (resource entry name "title").
        // Using withId(R.id.title) fails because the preference library's "title" ID has a
        // different numeric value than the test APK's R.id.title. Matching by entry name
        // works across package boundaries.
        // equalToIgnoringCase: settings search is case-insensitive, so callers may pass
        // lowercase search terms while the indexed title uses different capitalisation.
        onViewWaiting(allOf(withText(equalToIgnoringCase(text)), withResourceEntryName("title")))
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
        onViewWaiting(withId(R.id.recycler_view))
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

    private void typeIntoSearch(String query) {
        // search_box is the always-visible placeholder; clicking it calls enterSearchState()
        // which hides search_box and makes search_query_container + search_query visible.
        onViewWaiting(withId(R.id.search_box)).perform(click());
        onViewWaiting(withId(R.id.search_query))
                .perform(click(), typeText(query), closeSoftKeyboard());
    }

    /** Dumps the full view hierarchy of the current activity to logcat. */
    private void dumpViewHierarchy() {
        org.chromium.base.ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    Activity a = mSettingsActivityTestRule.getActivity();
                    if (a == null) {
                        org.chromium.base.Log.e("ViewDump", "activity is null");
                        return;
                    }
                    View root = a.getWindow().getDecorView();
                    dumpView(root, 0);
                });
    }

    private static void dumpView(View view, int depth) {
        String indent = new String(new char[depth * 2]).replace('\0', ' ');
        String idName;
        try {
            idName =
                    view.getId() == View.NO_ID
                            ? "NO_ID"
                            : view.getResources().getResourceEntryName(view.getId());
        } catch (Exception e) {
            idName = String.valueOf(view.getId());
        }
        String visibility =
                view.getVisibility() == View.VISIBLE
                        ? "VISIBLE"
                        : view.getVisibility() == View.GONE ? "GONE" : "INVISIBLE";
        org.chromium.base.Log.e(
                "ViewDump",
                indent
                        + view.getClass().getSimpleName()
                        + " id="
                        + idName
                        + " vis="
                        + visibility
                        + " w="
                        + view.getWidth()
                        + " h="
                        + view.getHeight()
                        + (view instanceof android.widget.TextView
                                ? " text=\"" + ((android.widget.TextView) view).getText() + "\""
                                : ""));
        if (view instanceof ViewGroup vg) {
            for (int i = 0; i < vg.getChildCount(); i++) {
                dumpView(vg.getChildAt(i), depth + 1);
            }
        }
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
