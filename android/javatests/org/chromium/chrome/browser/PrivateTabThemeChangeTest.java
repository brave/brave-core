// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser;

import static org.junit.Assert.assertTrue;

import androidx.test.filters.MediumTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.lifecycle.Stage;
import androidx.test.uiautomator.UiDevice;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.io.IOException;

/** Regression test for a private-tab crash during a system light or dark mode change. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableFeatures(ChromeFeatureList.ANDROID_OPEN_INCOGNITO_AS_WINDOW)
@DoNotBatch(
        reason = "Changes the device-wide system night mode and recreates ChromeTabbedActivity.")
public class PrivateTabThemeChangeTest {
    private static final String NIGHT_MODE_COMMAND = "cmd uimode night";
    private static final String NIGHT_MODE_PREFIX = "Night mode: ";
    private static final String NIGHT_MODE_ENABLED = "yes";
    private static final String NIGHT_MODE_DISABLED = "no";

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    private UiDevice mDevice;
    private String mInitialNightMode;

    @Before
    public void setUp() throws IOException {
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mInitialNightMode = getNightMode();

        mActivityTestRule.startMainActivityOnBlankPage();
        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());
    }

    @After
    public void tearDown() throws IOException {
        if (!getNightMode().equals(mInitialNightMode)) {
            setNightMode(mInitialNightMode);
        }
    }

    @Test
    @MediumTest
    public void testPrivateTabSurvivesSystemNightModeChanges() throws IOException {
        recreateActivityForNightMode(
                mInitialNightMode.equals(NIGHT_MODE_ENABLED)
                        ? NIGHT_MODE_DISABLED
                        : NIGHT_MODE_ENABLED);
        waitForActivityTab();

        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());

        recreateActivityForNightMode(mInitialNightMode);
        waitForActivityTab();
    }

    private void recreateActivityForNightMode(String nightMode) {
        ChromeTabbedActivity recreatedActivity =
                ApplicationTestUtils.waitForActivityWithClass(
                        ChromeTabbedActivity.class,
                        Stage.RESUMED,
                        /* uiThreadTrigger= */ null,
                        () -> {
                            try {
                                setNightMode(nightMode);
                            } catch (IOException e) {
                                throw new AssertionError(
                                        "Could not change the system night mode", e);
                            }
                        });
        mActivityTestRule.setActivity(recreatedActivity);
    }

    private void waitForActivityTab() {
        CriteriaHelper.pollUiThread(() -> mActivityTestRule.getActivityTab() != null);
    }

    private String getNightMode() throws IOException {
        String output = mDevice.executeShellCommand(NIGHT_MODE_COMMAND).trim();
        return output.substring(NIGHT_MODE_PREFIX.length());
    }

    private void setNightMode(String nightMode) throws IOException {
        mDevice.executeShellCommand(NIGHT_MODE_COMMAND + " " + nightMode);
    }
}
