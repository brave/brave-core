/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Unit tests for {@link BraveCommandLineInitUtil}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveCommandLineInitUtilTest {
    private static final String PREF_QA_COMMAND_LINE = "qa_command_line";
    private static final String SWITCH_VARIATIONS_PR = "variations-pr";
    private static final String SWITCH_ACCEPT_EMPTY_SIGNATURE =
            "accept-empty-variations-seed-signature";

    @Before
    public void setUp() {
        // Reset CommandLine to a clean empty state so each test starts fresh.
        CommandLine.resetForTesting(/* initialize= */ true);
        clearQaPref();
    }

    @After
    public void tearDown() {
        clearQaPref();
    }

    @Test
    public void testVariationsPrAddsAcceptEmptySeedSignature() {
        // Simulate the user setting --variations-pr= via BraveQAPreferences.
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putString(PREF_QA_COMMAND_LINE, "--" + SWITCH_VARIATIONS_PR + "=1739")
                .apply();

        BraveCommandLineInitUtil.appendBraveSwitchesAndArguments();

        assertTrue(
                "variations-pr should be present after reading qa_command_line pref",
                CommandLine.getInstance().hasSwitch(SWITCH_VARIATIONS_PR));
        assertTrue(
                "accept-empty-variations-seed-signature must be added when variations-pr is set,"
                        + " so that unsigned PR test seeds pass verification during early feature"
                        + " list initialization (before BraveMainDelegate::BasicStartupComplete)",
                CommandLine.getInstance().hasSwitch(SWITCH_ACCEPT_EMPTY_SIGNATURE));
    }

    @Test
    public void testWithoutVariationsPrNoAcceptEmptySeedSignature() {
        // No variations-pr switch set anywhere.
        BraveCommandLineInitUtil.appendBraveSwitchesAndArguments();

        assertFalse(
                "accept-empty-variations-seed-signature should not be added without variations-pr",
                CommandLine.getInstance().hasSwitch(SWITCH_ACCEPT_EMPTY_SIGNATURE));
    }

    private void clearQaPref() {
        ContextUtils.getAppSharedPreferences().edit().remove(PREF_QA_COMMAND_LINE).apply();
    }
}
