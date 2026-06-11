/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.action.ViewActions.closeSoftKeyboard;
import static androidx.test.espresso.action.ViewActions.typeText;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.CoreMatchers.allOf;

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
    public void testBraveSettingsAreSearchable() {
        mSettingsActivityTestRule.startSettingsActivity();

        // Search for "Shields" → expect Brave Shields & privacy
        typeIntoSearch("Shields");
        assertSearchResult("Brave Shields & privacy");

        // Search for "Firewall" → expect Brave Firewall + VPN
        clearAndTypeIntoSearch("Firewall");
        assertSearchResult("Brave Firewall + VPN");

        // Search for "Brave Sync" → expect Sync entry
        // clearAndTypeIntoSearch("Brave Sync");
        // assertSearchResult("Sync");
    }

    // ---------------------------------------------------------------------------
    // Category 2 — Chrome settings removed by Brave must NOT appear in search
    // ---------------------------------------------------------------------------

    /** Stub — to be implemented. */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testRemovedChromeSettingsNotFoundInSearch() {
        // TODO: implement — verify e.g. "Gemini in Brave", "Safety check" do not appear.
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
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    /**
     * Asserts that a search result with the given text is visible, explicitly excluding the search
     * query EditText which may also contain the same text as an autocomplete suggestion.
     */
    private void assertSearchResult(String text) {
        dumpViewHierarchy();
        // Match specifically the preference title TextView (resource entry name "title").
        // Using withId(R.id.title) fails because the preference library's "title" ID has a
        // different numeric value than the test APK's R.id.title. Matching by entry name
        // works across package boundaries.
        onViewWaiting(allOf(withText(text), withResourceEntryName("title")))
                .check(matches(isDisplayed()));
    }

    /** Matches a view whose resource entry name equals the given name. */
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
