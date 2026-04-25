/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crash;

import org.chromium.build.annotations.UsedByReflection;

import java.io.File;

@UsedByReflection("SplitCompatApplication.java")
public class BravePureJavaExceptionReporter extends ChromePureJavaExceptionReporter {
    @UsedByReflection("SplitCompatApplication.java")
    public BravePureJavaExceptionReporter() {
        super();
    }

    @Override
    protected void uploadMinidump(File minidump) {
        // Set `uploadNow` to false to run the job via JobScheduler instead of immediate upload as
        // we may get here while in background which causes issues with upload.
        LogcatExtractionRunnable.uploadMinidump(minidump, false);
    }
}
