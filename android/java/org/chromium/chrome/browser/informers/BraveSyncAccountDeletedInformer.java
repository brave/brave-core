/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.informers;

import androidx.core.content.ContextCompat;

import org.jni_zero.CalledByNative;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.settings.BraveSyncScreensPreference;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.components.messages.DismissReason;
import org.chromium.components.messages.MessageBannerProperties;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.messages.MessageDispatcherProvider;
import org.chromium.components.messages.MessageIdentifier;
import org.chromium.components.messages.PrimaryActionClickBehavior;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

// The legacy Android infobar UI was removed upstream
// (https://chromium-review.googlesource.com/c/chromium/src/+/7887741); this notice is now
// shown with the Messages framework. The inline "re-create the account" link of the old
// infobar becomes the message's primary button.
public class BraveSyncAccountDeletedInformer {
    private static final String TAG = "SyncAccountDeleted";

    @CalledByNative
    public static void show() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();

            Tab tab = activity.getActivityTabProvider().get();
            if (tab == null) return;

            WebContents webContents = tab.getWebContents();
            if (webContents == null) return;

            if (!BraveSyncWorker.get().isAccountDeletedNoticePending()) {
                return;
            }

            WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
            if (windowAndroid == null) return;
            MessageDispatcher messageDispatcher = MessageDispatcherProvider.from(windowAndroid);
            if (messageDispatcher == null) return;

            // The warning drawable has a fixed fill color, so tint it for the active theme,
            // matching the appearance the old infobar builder produced.
            final int iconColor =
                    GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                            ? ContextCompat.getColor(
                                    activity, R.color.brave_informer_dark_theme_icon_color)
                            : ContextCompat.getColor(
                                    activity, R.color.brave_informer_light_theme_icon_color);

            PropertyModel message =
                    new PropertyModel.Builder(MessageBannerProperties.ALL_KEYS)
                            .with(
                                    MessageBannerProperties.MESSAGE_IDENTIFIER,
                                    MessageIdentifier.INVALID_MESSAGE)
                            .with(MessageBannerProperties.DISMISSAL_DURATION, Integer.MAX_VALUE)
                            .with(
                                    MessageBannerProperties.DESCRIPTION,
                                    activity.getString(
                                            R.string.brave_sync_account_deleted_infobar_message))
                            .with(
                                    MessageBannerProperties.ICON_RESOURCE_ID,
                                    R.drawable.ic_warning_circle)
                            .with(MessageBannerProperties.ICON_TINT_COLOR, iconColor)
                            .with(
                                    MessageBannerProperties.PRIMARY_BUTTON_TEXT,
                                    activity.getString(
                                            R.string.brave_sync_account_deleted_infobar_link_text))
                            .with(
                                    MessageBannerProperties.ON_PRIMARY_ACTION,
                                    () -> {
                                        // Pressing `re-create the account` opens Sync settings.
                                        // The notice is disabled below via ON_DISMISSED, which the
                                        // framework invokes with DismissReason.PRIMARY_ACTION.
                                        SettingsNavigation settingsNavigation =
                                                SettingsNavigationFactory
                                                        .createSettingsNavigation();
                                        settingsNavigation.startSettings(
                                                ContextUtils.getApplicationContext(),
                                                BraveSyncScreensPreference.class);
                                        return PrimaryActionClickBehavior.DISMISS_IMMEDIATELY;
                                    })
                            .with(
                                    MessageBannerProperties.ON_DISMISSED,
                                    (dismissReason) -> {
                                        // Disable the informer on any explicit user action
                                        // (button, swipe, close). Automatic dismissals (timer,
                                        // scope destroyed, ...) leave the notice pending so it can
                                        // be shown again.
                                        if (isUserInitiatedDismiss(dismissReason)) {
                                            BraveSyncWorker.get()
                                                    .clearAccountDeletedNoticePending();
                                        }
                                    })
                            .build();

            messageDispatcher.enqueueWindowScopedMessage(message, /* highPriority= */ false);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "show " + e);
        }
    }

    private static boolean isUserInitiatedDismiss(@DismissReason int dismissReason) {
        return dismissReason == DismissReason.PRIMARY_ACTION
                || dismissReason == DismissReason.SECONDARY_ACTION
                || dismissReason == DismissReason.GESTURE
                || dismissReason == DismissReason.CLOSE_BUTTON;
    }
}
