// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.app;

import static org.hamcrest.Matchers.notNullValue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Criteria;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.IncognitoTabHostUtils;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTask;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskFeature;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskFeatureKey;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTaskTrackerFactory;
import org.chromium.chrome.browser.ui.extensions.windowing.ExtensionWindowControllerBridge;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

/** Tests the extension window bridge registration in {@link BraveActivity}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@EnableFeatures(BraveFeatureList.BRAVE_ANDROID_EXTENSIONS)
@DisableFeatures(ChromeFeatureList.ANDROID_OPEN_INCOGNITO_AS_WINDOW)
@DoNotBatch(reason = "Recreates ChromeTabbedActivity.")
public class BraveActivityWindowingFeaturesTest {
    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @MediumTest
    public void testBridgeUsesRegularProfile() {
        Tab incognitoTab = mActivityTestRule.newIncognitoTabFromMenu();
        assertTrue(incognitoTab.isIncognito());
        Profile incognitoProfile = incognitoTab.getProfile();

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    ChromeAndroidTask task = getChromeAndroidTask();
                    assertNotNull(getBridge(task, getRegularProfile()));
                    assertNull(getBridge(task, incognitoProfile));
                    assertEquals(1, countBridges(task));
                });
    }

    @Test
    @MediumTest
    public void testBridgeSurvivesClosingIncognitoTabsAfterRecreate() {
        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());

        ChromeTabbedActivity recreatedActivity =
                ApplicationTestUtils.recreateActivity(mActivityTestRule.getActivity());
        mActivityTestRule.setActivity(recreatedActivity);
        CriteriaHelper.pollUiThread(() -> mActivityTestRule.getActivityTab() != null);
        CriteriaHelper.pollUiThread(
                () ->
                        Criteria.checkThat(
                                getBridge(getChromeAndroidTask(), getRegularProfile()),
                                notNullValue()));

        // Destroys the incognito browser window.
        ThreadUtils.runOnUiThreadBlocking(IncognitoTabHostUtils::closeAllIncognitoTabs);
        CriteriaHelper.pollUiThread(
                () -> !mActivityTestRule.getActivity().getTabModelSelector().isIncognitoSelected());

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    ChromeAndroidTask task = getChromeAndroidTask();
                    assertNotNull(getBridge(task, getRegularProfile()));
                    assertEquals(1, countBridges(task));
                });
        assertNotNull(mActivityTestRule.getActivityTab());
    }

    private ChromeAndroidTask getChromeAndroidTask() {
        ChromeAndroidTask task =
                ChromeAndroidTaskTrackerFactory.getInstance()
                        .get(mActivityTestRule.getActivity().getTaskId());
        assertNotNull(task);
        return task;
    }

    private Profile getRegularProfile() {
        return mActivityTestRule
                .getActivity()
                .getTabModelSelector()
                .getModel(/* incognito= */ false)
                .getProfile();
    }

    private ChromeAndroidTaskFeature getBridge(ChromeAndroidTask task, Profile profile) {
        return task.getFeatureForTesting(
                new ChromeAndroidTaskFeatureKey(
                        ExtensionWindowControllerBridge.class,
                        profile,
                        mActivityTestRule.getActivity().getWindowAndroid()));
    }

    private static int countBridges(ChromeAndroidTask task) {
        int count = 0;
        for (ChromeAndroidTaskFeature feature : task.getAllFeaturesForTesting()) {
            if (feature instanceof ExtensionWindowControllerBridge) {
                count++;
            }
        }
        return count;
    }
}
