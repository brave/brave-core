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
import org.chromium.chrome.browser.settings.BraveSyncScreensPreference;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.snackbar.BraveSnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.components.browser_ui.settings.SettingsNavigation;

// The legacy Android infobar UI was removed upstream
// (https://chromium-review.googlesource.com/c/chromium/src/+/7887741); this notice is now
// shown with a bottom Snackbar. The inline "re-create the account" link of the old infobar
// becomes the snackbar's action button.
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

            SnackbarManager snackbarManager = activity.getSnackbarManager();
            if (snackbarManager == null) return;

            SnackbarController controller =
                    new SnackbarController() {
                        @Override
                        public void onAction(Object actionData) {
                            // Pressing `Re-create` opens Sync settings and disables the notice.
                            BraveSyncWorker.get().clearAccountDeletedNoticePending();
                            SettingsNavigation settingsNavigation =
                                    SettingsNavigationFactory.createSettingsNavigation();
                            settingsNavigation.startSettings(
                                    ContextUtils.getApplicationContext(),
                                    BraveSyncScreensPreference.class);
                        }

                        @Override
                        public void onDismissNoAction(Object actionData) {
                            BraveSyncWorker.get().clearAccountDeletedNoticePending();
                        }
                    };

            Snackbar snackbar =
                    Snackbar.make(
                                    activity.getString(
                                            R.string.brave_sync_account_deleted_infobar_message),
                                    controller,
                                    Snackbar.TYPE_PERSISTENT,
                                    Snackbar.UMA_UNKNOWN)
                            .setAction(
                                    activity.getString(
                                            R.string.brave_sync_account_deleted_infobar_link_text),
                                    /* actionData= */ null)
                            .setDefaultLines(false);

            snackbarManager.showSnackbar(snackbar);

            // Move the long action label onto its own line below the message, with a trailing close
            // button. Closing dismisses the snackbar, which triggers onDismissNoAction above and
            // disables the notice (the same as letting it sit).
            if (snackbarManager instanceof BraveSnackbarManager) {
                BraveSnackbarManager braveSnackbarManager = (BraveSnackbarManager) snackbarManager;
                braveSnackbarManager.setActionBelowMessage(
                        R.drawable.ic_close,
                        activity.getString(R.string.close),
                        () -> braveSnackbarManager.dismissSnackbars(controller));
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "show " + e);
        }
    }
}
