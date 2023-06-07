/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.Supplier;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public abstract class BraveCommandLineInitUtil {
    private static final String TAG = "BraveCommandLineInitUtil";

    // Duplicate constant to avoid pull dependancy into base
    private static final String PREF_QA_VLOG_REWARDS = "qa_vlog_rewards";
    private static final String PREF_QA_COMMAND_LINE = "qa_command_line";
    private static final String TEST_VARIATIONS_SERVER_URL_FILE =
            "/data/local/tmp/brave-test-variations-server-url";

    public static void initCommandLine(
            String fileName, @Nullable Supplier<Boolean> shouldUseDebugFlags) {
        CommandLineInitUtil.initCommandLine(fileName, shouldUseDebugFlags);
        appendBraveSwitchesAndArguments();
    }

    private static void appendBraveSwitchesAndArguments() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        String qaCommandLine = sharedPreferences.getString(PREF_QA_COMMAND_LINE, "");
        if (sharedPreferences.getBoolean(PREF_QA_VLOG_REWARDS, false)) {
            qaCommandLine += " --enable-logging=stderr";
            qaCommandLine +=
                    " --vmodule=*/brave_ads/*=6,*/brave_user_model/*=6,*/bat_ads/*=6,*/brave_rewards/*=6";
        }

        // For testing we need custom variations server url prior to the first time run, so we try
        // to read it from the file.
        File variationsServerFile = new File(TEST_VARIATIONS_SERVER_URL_FILE);
        if (variationsServerFile.exists()) {
            try (BufferedReader reader = new BufferedReader(new FileReader(variationsServerFile))) {
                String testVariationsServerUrl = reader.readLine();
                if (testVariationsServerUrl != null && !testVariationsServerUrl.isEmpty()) {
                    Log.w(TAG,
                            "New test variations server url applied: " + testVariationsServerUrl);
                    qaCommandLine += " --variations-server-url=" + testVariationsServerUrl;
                } else {
                    Log.w(TAG, "Variations server file appears to be empty");
                }
            } catch (IOException e) {
                Log.e(TAG, "Failed to read variations server file: " + e.getMessage());
            }
        }

        @SuppressLint("VisibleForTests")
        String[] args = CommandLine.tokenizeQuotedArguments(qaCommandLine.toCharArray());
        CommandLine.getInstance().appendSwitchesAndArguments(args);
    }
}
