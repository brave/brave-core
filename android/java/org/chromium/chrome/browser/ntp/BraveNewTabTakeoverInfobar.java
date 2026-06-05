/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import androidx.core.content.ContextCompat;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.messages.DismissReason;
import org.chromium.components.messages.MessageBannerProperties;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.messages.MessageDispatcherProvider;
import org.chromium.components.messages.MessageIdentifier;
import org.chromium.components.messages.MessageScopeType;
import org.chromium.components.messages.PrimaryActionClickBehavior;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

// The legacy Android infobar UI was removed upstream
// (https://chromium-review.googlesource.com/c/chromium/src/+/7887741); this notice is now
// shown with the Messages framework. The inline "learn more / opt out choices" link of the old
// infobar becomes the message's primary button.
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
        MessageDispatcher messageDispatcher = MessageDispatcherProvider.from(windowAndroid);
        if (messageDispatcher == null) return;

        recordInfobarWasDisplayed();

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
                                MessageBannerProperties.ICON_RESOURCE_ID,
                                R.drawable.ic_warning_circle)
                        .with(MessageBannerProperties.ICON_TINT_COLOR, iconColor)
                        .with(
                                MessageBannerProperties.DESCRIPTION,
                                activity.getString(R.string.new_tab_takeover_infobar_message))
                        .with(
                                MessageBannerProperties.PRIMARY_BUTTON_TEXT,
                                activity.getString(
                                        R.string
                                                .new_tab_takeover_infobar_learn_more_opt_out_choices))
                        .with(
                                MessageBannerProperties.ON_PRIMARY_ACTION,
                                () -> {
                                    // Pressing `Learn more / opt out choices` opens the support
                                    // page. The infobar is suppressed below via ON_DISMISSED, which
                                    // the framework invokes with DismissReason.PRIMARY_ACTION.
                                    TabUtils.openUrlInNewTab(
                                            /* isIncognito= */ false, LEARN_MORE_URL);
                                    return PrimaryActionClickBehavior.DISMISS_IMMEDIATELY;
                                })
                        .with(
                                MessageBannerProperties.ON_DISMISSED,
                                (dismissReason) -> {
                                    // Suppress on any explicit user action (button, swipe, close).
                                    // Automatic dismissals (timer, navigation, ...) only consume
                                    // one of the remaining displays, decremented above.
                                    if (isUserInitiatedDismiss(dismissReason)) {
                                        suppressInfobar();
                                    }
                                })
                        .build();

        messageDispatcher.enqueueMessage(
                message, webContents, MessageScopeType.NAVIGATION, /* highPriority */ false);
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

    private static boolean isUserInitiatedDismiss(@DismissReason int dismissReason) {
        return dismissReason == DismissReason.PRIMARY_ACTION
                || dismissReason == DismissReason.SECONDARY_ACTION
                || dismissReason == DismissReason.GESTURE
                || dismissReason == DismissReason.CLOSE_BUTTON;
    }
}
