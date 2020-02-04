/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.document;

import android.app.Activity;
import android.os.Bundle;

import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.flags.FeatureUtilities;

/**
 * Base class for ChromeLauncherActivity
 */
public class BraveLauncherActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Disable FTR experience
        CommandLine.getInstance().appendSwitch(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE);
        FirstRunStatus.setFirstRunFlowComplete(true);

        FeatureUtilities.isBottomToolbarEnabled();
    }
}
