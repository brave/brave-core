/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.messages.snackbar.BraveSnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

// The legacy Android infobar UI was removed upstream
// (https://chromium-review.googlesource.com/c/chromium/src/+/7887741); this notice is now
// shown with a bottom Snackbar. The inline "learn more / opt out choices" link of the old infobar
// becomes the snackbar's action button. The action label is long, so the snackbar is switched to
// the Brave action-below-message layout (BraveSnackbarManager#setActionBelowMessage).
@NullMarked
public class BraveNewTabTakeoverInfobar {
    private static final String LEARN_MORE_URL =
            "https://support.brave.app/hc/en-us/articles/35182999599501";
    private final Profile mProfile;

    public BraveNewTabTakeoverInfobar(Profile profile) {
        mProfile = profile;
    }

    public void maybeDisplayAndIncrementCounter(Activity activity, WebContents webContents) {
        if (!shouldDisplayInfobar()) {
            return;
        }

        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) return;
        SnackbarManager snackbarManager = SnackbarManagerProvider.from(windowAndroid);
        if (snackbarManager == null) return;

        recordInfobarWasDisplayed();

        SnackbarController controller =
                new SnackbarController() {
                    @Override
                    public void onAction(@Nullable Object actionData) {
                        // Pressing `Learn more` opens the support page and stops the notice from
                        // showing again.
                        suppressInfobar();
                        TabUtils.openUrlInNewTab(/* isIncognito= */ false, LEARN_MORE_URL);
                    }
                };

        Snackbar snackbar =
                Snackbar.make(
                                activity.getString(R.string.new_tab_takeover_infobar_message),
                                controller,
                                Snackbar.TYPE_PERSISTENT,
                                Snackbar.UMA_UNKNOWN)
                        .setAction(
                                activity.getString(
                                        R.string
                                                .new_tab_takeover_infobar_learn_more_opt_out_choices),
                                /* actionData= */ null)
                        .setDefaultLines(false);

        snackbarManager.showSnackbar(snackbar);

        // Move the long action label onto its own line below the message, with a trailing close
        // button. Closing just dismisses the snackbar and does not suppress future displays, so the
        // notice can still be shown up to its remaining display count.
        if (snackbarManager instanceof BraveSnackbarManager) {
            BraveSnackbarManager braveSnackbarManager = (BraveSnackbarManager) snackbarManager;
            braveSnackbarManager.setActionBelowMessage(
                    R.drawable.ic_close,
                    activity.getString(R.string.close),
                    () -> braveSnackbarManager.dismissSnackbars(controller));
        }
    }

    private boolean shouldDisplayInfobar() {
        if (BraveRewardsHelper.isRewardsEnabled()) {
            return false;
        }

        PrefService prefService = UserPrefs.get(mProfile);
        final int infobarDisplayCount =
                prefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT);
        return infobarDisplayCount > 0;
    }

    private void recordInfobarWasDisplayed() {
        PrefService prefService = UserPrefs.get(mProfile);
        final int infobarDisplayCount =
                prefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT);
        prefService.setInteger(
                BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT,
                infobarDisplayCount - 1);
    }

    private void suppressInfobar() {
        PrefService prefService = UserPrefs.get(mProfile);
        prefService.setInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT, 0);
    }
}
