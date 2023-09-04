/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.infobar.BraveInfoBarIdentifier;
import org.chromium.chrome.browser.settings.BraveSyncScreensPreference;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.infobar.BraveSimpleConfirmInfoBarBuilder;
import org.chromium.chrome.browser.ui.messages.infobar.SimpleConfirmInfoBarBuilder;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

public class BraveSyncInformers {
    private static final String TAG = "BraveSyncInformers";

    public static void show() {
        showSetupV2IfRequired();
    }

    private static void showSetupV2IfRequired() {
        BraveSyncWorker braveSyncWorker = BraveSyncWorker.get();
        boolean wasV1User = braveSyncWorker.getSyncV1WasEnabled();

        if (!wasV1User) {
            return;
        }

        boolean infobarDismissed = braveSyncWorker.getSyncV2MigrateNoticeDismissed();
        if (infobarDismissed) {
            return;
        }

        boolean isV2User = SyncServiceFactory.get() != null
                && SyncServiceFactory.get().isInitialSyncFeatureSetupComplete();
        if (isV2User) {
            braveSyncWorker.setSyncV2MigrateNoticeDismissed(true);
            return;
        }

        showSyncV2NeedsSetup();
    }

    public static void showSyncV2NeedsSetup() {
        BraveActivity activity = null;
        try {
            activity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showSyncV2NeedsSetup " + e);
            return;
        }

        Tab tab = activity.getActivityTabProvider().get();
        if (tab == null) return;

        BraveSimpleConfirmInfoBarBuilder.createInfobarWithDrawable(tab.getWebContents(),
                new SimpleConfirmInfoBarBuilder.Listener() {
                    @Override
                    public void onInfoBarDismissed() {
                    }

                    @Override
                    public boolean onInfoBarButtonClicked(boolean isPrimary) {
                        if (isPrimary) {
                            SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
                            settingsLauncher.launchSettingsActivity(
                                ContextUtils.getApplicationContext(),
                                BraveSyncScreensPreference.class);
                        }
                        return false;
                    }

                    @Override
                    public boolean onInfoBarLinkClicked() {
                        return false;
                    }
                },
                BraveInfoBarIdentifier.SYNC_V2_MIGRATE_INFOBAR_DELEGATE, activity,
                R.drawable.sync_icon /* drawableId */,
                activity.getString(R.string.brave_sync_v2_migrate_infobar_message) /* message */,
                activity.getString(
                        R.string.brave_sync_v2_migrate_infobar_command) /* primaryText */,
                null /* secondaryText */, null /* linkText */, false /* autoExpire */);
        BraveSyncWorker.get().setSyncV2MigrateNoticeDismissed(true);
    }
}
