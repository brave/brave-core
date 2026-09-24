/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.embedder_support.view;

import android.content.Context;
import android.view.ViewStructure;

import org.chromium.base.JavaExceptionReporter;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.content_public.browser.WebContents;

/** That class extends upstream's ContentView.java to prevent a browser process crash */
public class BraveContentView extends ContentView {
    protected BraveContentView(Context context, WebContents webContents) {
        super(context, webContents);
    }

    @Override
    public void onProvideAutofillVirtualStructure(ViewStructure structure, int flags) {
        try {
            super.onProvideAutofillVirtualStructure(structure, flags);
        } catch (NullPointerException exception) {
            // We still want the stack to be reported with a hope to fix that issue
            // one day. See https://github.com/brave/brave-browser/issues/37942 for
            // details
            PostTask.postTask(
                    TaskTraits.UI_BEST_EFFORT,
                    () -> JavaExceptionReporter.reportException(exception));
        }
    }
}
