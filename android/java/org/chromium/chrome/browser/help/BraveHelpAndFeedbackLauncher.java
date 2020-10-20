/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.help;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;

import org.chromium.base.Log;
import org.chromium.chrome.browser.feedback.FeedbackCollector;
import org.chromium.chrome.browser.feedback.HelpAndFeedbackLauncherImpl;

import javax.annotation.Nonnull;

public class BraveHelpAndFeedbackLauncher extends HelpAndFeedbackLauncherImpl {
    protected static final String FALLBACK_SUPPORT_URL = "https://community.brave.com/";
    private static final String TAG = "BraveHelpAndFeedbackLauncher";

    @Override
    protected void show(
            Activity activity, String helpContext, @Nonnull FeedbackCollector collector) {
        Log.d(TAG, "Feedback data: " + collector.getBundle());
        launchFallbackSupportUri(activity);
    }

    @Override
    protected void showFeedback(Activity activity, @Nonnull FeedbackCollector collector) {
        Log.d(TAG, "Feedback data: " + collector.getBundle());
        launchFallbackSupportUri(activity);
    }

    protected static void launchFallbackSupportUri(Context context) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(FALLBACK_SUPPORT_URL));
        // Let Chrome know that this intent is from Chrome, so that it does not close the app when
        // the user presses 'back' button.
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);
        intent.setPackage(context.getPackageName());
        context.startActivity(intent);
    }
}
