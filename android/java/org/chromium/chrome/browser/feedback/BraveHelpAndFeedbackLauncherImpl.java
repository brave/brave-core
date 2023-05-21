/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.feedback;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.Map;

import javax.annotation.Nonnull;

public class BraveHelpAndFeedbackLauncherImpl implements HelpAndFeedbackLauncher {
    private static final String FALLBACK_SUPPORT_URL = "https://community.brave.com/";
    private static final String SAFE_BROWSING_URL =
            "https://brave.com/privacy/browser/#safe-browsing";

    public BraveHelpAndFeedbackLauncherImpl(Profile profile) {}

    @Override
    public void show(final Activity activity, String helpContext, @Nullable String url) {
        if (helpContext.equalsIgnoreCase(
                    activity.getResources().getString(R.string.help_context_safe_browsing))) {
            launchSafeBrowsingUri(activity);
        } else {
            launchFallbackSupportUri(activity);
        }
    }

    @Override
    public void showFeedback(final Activity activity, @Nullable String url,
            @Nullable final String categoryTag, int screenshotMode,
            @Nullable final String feedbackContext) {
        showFeedback(activity, url, categoryTag);
    }

    @Override
    public void showFeedback(
            final Activity activity, @Nullable String url, @Nullable final String categoryTag) {
        launchFallbackSupportUri(activity);
    }

    @Override
    public void showFeedback(final Activity activity, @Nullable String url,
            @Nullable final String categoryTag, @Nullable final Map<String, String> feedContext) {
        showFeedback(activity, url, categoryTag);
    }

    private static void launchFallbackSupportUri(Context context) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(FALLBACK_SUPPORT_URL));
        // Let Chrome know that this intent is from Chrome, so that it does not close the app when
        // the user presses 'back' button.
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);
        intent.setPackage(context.getPackageName());
        context.startActivity(intent);
    }

    private void launchSafeBrowsingUri(Context context) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(SAFE_BROWSING_URL));
        // Let Brave know that this intent is from Brave, so that it does not close the app when
        // the user presses 'back' button.
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);
        intent.setPackage(context.getPackageName());
        context.startActivity(intent);
    }
}
