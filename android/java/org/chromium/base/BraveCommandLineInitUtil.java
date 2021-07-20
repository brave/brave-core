/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;

import androidx.annotation.Nullable;

import org.chromium.base.CommandLine;
import org.chromium.base.CommandLineInitUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.Supplier;

import java.util.ArrayList;

public abstract class BraveCommandLineInitUtil {
    // Duplicate constant to avoid pull dependancy into base
    private static final String PREF_QA_VLOG_REWARDS = "qa_vlog_rewards";
    private static final String PREF_QA_COMMAND_LINE = "qa_command_line";

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
                    " --vmodule=*/bat-native-ads/*=6,*/brave_ads/*=6,*/brave_user_model/*=6,*/bat_ads/*=6,*/bat-native-ledger/*=6,*/brave_rewards/*=6";
        }
        @SuppressLint("VisibleForTests")
        String[] args = CommandLine.tokenizeQuotedArguments(qaCommandLine.toCharArray());
        CommandLine.getInstance().appendSwitchesAndArguments(args);
    }
}
