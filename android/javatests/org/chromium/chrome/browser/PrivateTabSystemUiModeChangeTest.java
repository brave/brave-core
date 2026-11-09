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

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Criteria;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.app.tabmodel.AsyncTabParamsManagerSingleton;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Regression tests for private-tab crashes during system UI mode changes. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableFeatures(ChromeFeatureList.ANDROID_OPEN_INCOGNITO_AS_WINDOW)
@DoNotBatch(
        reason =
                "Changes device-wide night mode and battery saver state, and recreates"
                        + " ChromeTabbedActivity.")
public class PrivateTabSystemUiModeChangeTest {
    private static final String ABOUT_BLANK_URL = "about:blank";
    private static final String NIGHT_MODE_COMMAND = "cmd uimode night";
    private static final String NIGHT_MODE_PREFIX = "Night mode: ";
    private static final String NIGHT_MODE_ENABLED = "yes";
    private static final String NIGHT_MODE_DISABLED = "no";
    private static final String BATTERY_SAVER_MODE_COMMAND = "cmd power set-mode ";
    private static final String BATTERY_SAVER_MODE_SETTING = "settings get global low_power";
    private static final String BATTERY_SAVER_MODE_ENABLED = "1";
    private static final String BATTERY_SAVER_MODE_DISABLED = "0";
    private static final String BATTERY_UNPLUG_COMMAND = "cmd battery unplug";
    private static final String BATTERY_RESET_COMMAND = "cmd battery reset";
    private static final String UI_MODE_DUMP_COMMAND = "dumpsys uimode";
    private static final String CURRENT_UI_MODE_PREFIX = "mCurUiMode=0x";

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    private UiDevice mDevice;
    private String mInitialNightMode;
    private String mInitialBatterySaverMode;
    private boolean mBatteryUnpluggedForTest;

    @Before
    public void setUp() throws IOException {
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mInitialNightMode = getNightMode();
        mInitialBatterySaverMode = getBatterySaverMode();
        prepareSystemUiModeBaseline();

        mActivityTestRule.startMainActivityOnBlankPage();
        // Saving this tab creates the private cipher key. A private NTP alone never does, so the
        // recreated activity would skip private tabs and leak the stashed one (known issue).
        assertTrue(
                mActivityTestRule
                        .loadUrlInNewTab(ABOUT_BLANK_URL, /* incognito= */ true)
                        .isIncognito());
        assertTrue(mActivityTestRule.newIncognitoTabFromMenu().isIncognito());
    }

    @After
    public void tearDown() throws IOException {
        // Restore Battery Saver before resetting simulated battery state: Android rejects enabling
        // it while the device reports external power.
        try {
            if (mInitialBatterySaverMode != null
                    && !getBatterySaverMode().equals(mInitialBatterySaverMode)) {
                setBatterySaverMode(mInitialBatterySaverMode);
            }
        } finally {
            try {
                if (mBatteryUnpluggedForTest) {
                    mDevice.executeShellCommand(BATTERY_RESET_COMMAND);
                }
            } finally {
                // Restore night mode even if Battery Saver or battery-state restoration fails.
                if (mInitialNightMode != null && !getNightMode().equals(mInitialNightMode)) {
                    setNightMode(mInitialNightMode);
                }
            }
        }
    }

    @Test
    @MediumTest
    public void testPrivateTabDoesNotCrashAfterSystemNightModeChange() throws IOException {
        List<Integer> privateTabIds = getPrivateTabIds();

        recreateActivityForNightMode(NIGHT_MODE_ENABLED);
        waitForPrivateTabsReparented(privateTabIds);

        privateTabIds.add(mActivityTestRule.newIncognitoTabFromMenu().getId());

        recreateActivityForNightMode(NIGHT_MODE_DISABLED);
        waitForPrivateTabsReparented(privateTabIds);
    }

    @Test
    @MediumTest
    public void testBackgroundPrivateTabsSurviveSystemNightModeChange() throws IOException {
        List<Integer> privateTabIds = getPrivateTabIds();
        privateTabIds.add(mActivityTestRule.newIncognitoTabFromMenu().getId());
        ThreadUtils.runOnUiThreadBlocking(
                () -> mActivityTestRule.getActivity().getTabModelSelector().selectModel(false));

        recreateActivityForNightMode(NIGHT_MODE_ENABLED);
        waitForPrivateTabsReparented(privateTabIds);

        recreateActivityForNightMode(NIGHT_MODE_DISABLED);
        waitForPrivateTabsReparented(privateTabIds);
    }

    @Test
    @MediumTest
    public void testPrivateTabDoesNotCrashAfterBatterySaverModeChange() throws IOException {
        // Battery Saver cannot be enabled while the device reports external power.
        mDevice.executeShellCommand(BATTERY_UNPLUG_COMMAND);
        mBatteryUnpluggedForTest = true;
        List<Integer> privateTabIds = getPrivateTabIds();

        recreateActivityForBatterySaverMode(BATTERY_SAVER_MODE_ENABLED);
        // The real system configuration change triggers the activity recreation under test.
        assertTrue(
                "Battery saver did not switch the system UI mode to night",
                isSystemNightModeEnabled());
        waitForPrivateTabsReparented(privateTabIds);

        privateTabIds.add(mActivityTestRule.newIncognitoTabFromMenu().getId());

        recreateActivityForBatterySaverMode(BATTERY_SAVER_MODE_DISABLED);
        assertFalse(
                "Battery saver did not restore the system UI mode to light",
                isSystemNightModeEnabled());
        waitForPrivateTabsReparented(privateTabIds);
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

    private List<Integer> getPrivateTabIds() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    List<Integer> ids = new ArrayList<>();
                    for (Tab tab :
                            mActivityTestRule
                                    .getActivity()
                                    .getTabModelSelector()
                                    .getModel(/* incognito= */ true)) {
                        ids.add(tab.getId());
                    }
                    return ids;
                });
    }

    // A private tab left out of the restore stays stashed for reparenting, keeping its WebContents.
    private void waitForPrivateTabsReparented(List<Integer> expectedIds) {
        CriteriaHelper.pollUiThread(
                () -> {
                    Criteria.checkThat(mActivityTestRule.getActivityTab(), Matchers.notNullValue());
                    Criteria.checkThat(
                            getPrivateTabIds(), Matchers.containsInAnyOrder(expectedIds.toArray()));
                    Criteria.checkThat(
                            AsyncTabParamsManagerSingleton.getInstance().getAsyncTabParams().size(),
                            Matchers.is(0));
                });
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
