// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.res.Configuration;

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

/** Regression tests for private-tab crashes during system UI mode changes. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableFeatures(ChromeFeatureList.ANDROID_OPEN_INCOGNITO_AS_WINDOW)
@DoNotBatch(
        reason =
                "Changes device-wide night mode and battery saver state, and recreates"
                        + " ChromeTabbedActivity.")
public class PrivateTabSystemUiModeChangeTest {
    private static final String NIGHT_MODE_COMMAND = "cmd uimode night";
    private static final String NIGHT_MODE_PREFIX = "Night mode: ";
    private static final String NIGHT_MODE_ENABLED = "yes";
    private static final String NIGHT_MODE_DISABLED = "no";
    private static final String BATTERY_SAVER_MODE_COMMAND = "cmd power set-mode ";
    private static final String BATTERY_SAVER_MODE_SETTING = "settings get global low_power";
    private static final String BATTERY_SAVER_MODE_ENABLED = "1";
    private static final String BATTERY_SAVER_MODE_DISABLED = "0";
    private static final String UI_MODE_DUMP_COMMAND = "dumpsys uimode";
    private static final String CURRENT_UI_MODE_PREFIX = "mCurUiMode=0x";

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    private UiDevice mDevice;
    private String mInitialNightMode;
    private String mInitialBatterySaverMode;

    @Before
    public void setUp() throws IOException {
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mInitialNightMode = getNightMode();
        mInitialBatterySaverMode = getBatterySaverMode();
        prepareSystemUiModeBaseline();

        mActivityTestRule.startMainActivityOnBlankPage();
        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());
    }

    @After
    public void tearDown() throws IOException {
        try {
            if (mInitialBatterySaverMode != null
                    && !getBatterySaverMode().equals(mInitialBatterySaverMode)) {
                setBatterySaverMode(mInitialBatterySaverMode);
            }
        } finally {
            // Restore night mode even if battery-saver restoration fails.
            if (mInitialNightMode != null && !getNightMode().equals(mInitialNightMode)) {
                setNightMode(mInitialNightMode);
            }
        }
    }

    @Test
    @MediumTest
    public void testPrivateTabDoesNotCrashAfterSystemNightModeChange() throws IOException {
        recreateActivityForNightMode(NIGHT_MODE_ENABLED);
        waitForActivityTab();

        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());

        recreateActivityForNightMode(NIGHT_MODE_DISABLED);
        waitForActivityTab();
    }

    @Test
    @MediumTest
    public void testPrivateTabDoesNotCrashAfterBatterySaverModeChange() throws IOException {
        recreateActivityForBatterySaverMode(BATTERY_SAVER_MODE_ENABLED);
        // The real system configuration change triggers the activity recreation under test.
        assertTrue(
                "Battery saver did not switch the system UI mode to night",
                isSystemNightModeEnabled());
        waitForActivityTab();

        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());

        recreateActivityForBatterySaverMode(BATTERY_SAVER_MODE_DISABLED);
        assertFalse(
                "Battery saver did not restore the system UI mode to light",
                isSystemNightModeEnabled());
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

    private void recreateActivityForBatterySaverMode(String batterySaverMode) {
        ChromeTabbedActivity recreatedActivity =
                ApplicationTestUtils.waitForActivityWithClass(
                        ChromeTabbedActivity.class,
                        Stage.RESUMED,
                        /* uiThreadTrigger= */ null,
                        () -> {
                            try {
                                setBatterySaverMode(batterySaverMode);
                            } catch (IOException e) {
                                throw new AssertionError(
                                        "Could not change the battery saver mode", e);
                            }
                        });
        mActivityTestRule.setActivity(recreatedActivity);
    }

    private void waitForActivityTab() {
        CriteriaHelper.pollUiThread(() -> mActivityTestRule.getActivityTab() != null);
    }

    private String getNightMode() throws IOException {
        String output = mDevice.executeShellCommand(NIGHT_MODE_COMMAND).trim();
        if (!output.startsWith(NIGHT_MODE_PREFIX)) {
            throw new AssertionError("Unexpected system night mode output: " + output);
        }
        return output.substring(NIGHT_MODE_PREFIX.length());
    }

    private void setNightMode(String nightMode) throws IOException {
        mDevice.executeShellCommand(NIGHT_MODE_COMMAND + " " + nightMode);
        assertEquals(nightMode, getNightMode());
    }

    private String getBatterySaverMode() throws IOException {
        String batterySaverMode = mDevice.executeShellCommand(BATTERY_SAVER_MODE_SETTING).trim();
        if (!batterySaverMode.equals(BATTERY_SAVER_MODE_ENABLED)
                && !batterySaverMode.equals(BATTERY_SAVER_MODE_DISABLED)) {
            throw new AssertionError("Unexpected battery saver mode: " + batterySaverMode);
        }
        return batterySaverMode;
    }

    private void setBatterySaverMode(String batterySaverMode) throws IOException {
        mDevice.executeShellCommand(BATTERY_SAVER_MODE_COMMAND + batterySaverMode);
        assertEquals(batterySaverMode, getBatterySaverMode());
    }

    private void prepareSystemUiModeBaseline() throws IOException {
        setBatterySaverMode(BATTERY_SAVER_MODE_DISABLED);
        setNightMode(NIGHT_MODE_DISABLED);
        assertFalse("Could not prepare a light system UI mode", isSystemNightModeEnabled());
    }

    private boolean isSystemNightModeEnabled() throws IOException {
        String uiModeDump = mDevice.executeShellCommand(UI_MODE_DUMP_COMMAND);
        int start = uiModeDump.indexOf(CURRENT_UI_MODE_PREFIX);
        if (start == -1) {
            throw new AssertionError("Could not find the current UI mode in: " + uiModeDump);
        }
        start += CURRENT_UI_MODE_PREFIX.length();
        int end = start;
        while (end < uiModeDump.length() && Character.digit(uiModeDump.charAt(end), 16) != -1) {
            end++;
        }
        int currentUiMode = Integer.parseInt(uiModeDump.substring(start, end), 16);
        return (currentUiMode & Configuration.UI_MODE_NIGHT_MASK)
                == Configuration.UI_MODE_NIGHT_YES;
    }
}
