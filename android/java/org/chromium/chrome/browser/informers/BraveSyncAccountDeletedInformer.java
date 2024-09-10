/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.informers;

import org.jni_zero.CalledByNative;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.infobar.BraveInfoBarIdentifier;
import org.chromium.chrome.browser.settings.BraveSyncScreensPreference;
import org.chromium.chrome.browser.settings.SettingsLauncherFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.infobar.BraveSimpleConfirmInfoBarBuilder;
import org.chromium.chrome.browser.ui.messages.infobar.SimpleConfirmInfoBarBuilder;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

public class BraveSyncAccountDeletedInformer {
    private static final String TAG = "SyncAccountDeleted";

    @CalledByNative
    public static void show() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();

            Tab tab = activity.getActivityTabProvider().get();
            if (tab == null) return;

            if (!BraveSyncWorker.get().isAccountDeletedNoticePending()) {
                return;
            }

            BraveSimpleConfirmInfoBarBuilder.createInfobarWithDrawable(
                    tab.getWebContents(),
                    new SimpleConfirmInfoBarBuilder.Listener() {
                        @Override
                        public void onInfoBarDismissed() {
                            // Pressing cross
                            // In any way don't show the informer again
                            disableInformer();
                        }

                        @Override
                        public boolean onInfoBarButtonClicked(boolean isPrimary) {
                            assert isPrimary : "We don't have secondary button";
                            // Pressing `OK`
                            // Don't show the informer again
                            disableInformer();
                            return false;
                        }

                        @Override
                        public boolean onInfoBarLinkClicked() {
                            // Pressing link `re-create the account`
                            // Don't show the informer again
                            disableInformer();
                            SettingsLauncher settingsLauncher =
                                    SettingsLauncherFactory.createSettingsLauncher();
                            settingsLauncher.launchSettingsActivity(
                                    ContextUtils.getApplicationContext(),
                                    BraveSyncScreensPreference.class);
                            return false;
                        }
                    },
                    BraveInfoBarIdentifier.BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR,
                    activity,
                    R.drawable.ic_warning_circle,
                    // See comment at |BraveSyncAccountDeletedInfoBarDelegate::GetMessageText|
                    // for the informer text and link test placeholder empty substitution
                    activity.getString(R.string.brave_sync_account_deleted_infobar_message, ""),
                    activity.getString(R.string.ok),
                    "",
                    activity.getString(R.string.brave_sync_account_deleted_infobar_link_text, ""),
                    false);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "show " + e);
        }
    }

    private static void disableInformer() {
        BraveSyncWorker.get().clearAccountDeletedNoticePending();
    }
}
