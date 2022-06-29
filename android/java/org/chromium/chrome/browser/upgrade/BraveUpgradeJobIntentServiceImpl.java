/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.SafetyNetCheck;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.site_settings.WebsitePreferenceBridge;
import org.chromium.components.content_settings.ContentSettingValues;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.UiThreadTaskTraits;

public class BraveUpgradeJobIntentServiceImpl extends BraveUpgradeJobIntentService.Impl {
    private static final String TAG = "BraveUpgradeJobIntentService";

    private static final int JOB_ID = 1;

    public static void maybePerformUpgradeTasks(Context context) {
        BraveUpgradeJobIntentServiceImpl.enqueueWork(context, new Intent());
    }

    private static void maybeMigrateSettings() {
        // False is the default value, so we want to migrate it only when it's true.
        if (BravePrefServiceBridge.getInstance().getDesktopModeEnabled()) {
            Profile profile = Profile.getLastUsedRegularProfile();
            WebsitePreferenceBridge.setDefaultContentSetting(
                    profile, ContentSettingsType.REQUEST_DESKTOP_SITE, ContentSettingValues.ALLOW);
            // Reset old flag to default value, so we don't migrate it anymore.
            BravePrefServiceBridge.getInstance().setDesktopModeEnabled(false);
        }
    }

    private static void enqueueWork(Context context, Intent work) {
        BraveUpgradeJobIntentService.Impl.enqueueWork(
                context, BraveUpgradeJobIntentService.class, JOB_ID, work);
    }

    @Override
    protected void onHandleWork(@NonNull Intent intent) {
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            BrowserStartupController.getInstance().addStartupCompletedObserver(
                    new BrowserStartupController.StartupCallback() {
                        @Override
                        public void onSuccess() {
                            maybeMigrateSettings();

                            if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                                    && BravePrefServiceBridge.getInstance()
                                               .getSafetynetCheckFailed()) {
                                Callback<Boolean> callback = value -> {
                                    if (value == null || !value.booleanValue()) {
                                        return;
                                    }
                                    // Reset flag and update UI
                                    BravePrefServiceBridge.getInstance().setSafetynetCheckFailed(
                                            false);
                                    TabUtils.enableRewardsButton();
                                };
                                // Re-perform safetynet check
                                SafetyNetCheck.updateSafetynetStatus(callback);
                            }
                        }

                        @Override
                        public void onFailure() {
                            Log.e(TAG,
                                    "Failed to perform upgrade tasks: BrowserStartupController.StartupCallback failed");
                        }
                    });
        });
    }
}
