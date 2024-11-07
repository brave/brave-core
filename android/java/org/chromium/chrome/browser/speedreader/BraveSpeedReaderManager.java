/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.speedreader;

import android.content.res.Resources;

import androidx.annotation.Nullable;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.UserData;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.components.messages.DismissReason;
import org.chromium.components.messages.MessageBannerProperties;
import org.chromium.components.messages.MessageDispatcher;
import org.chromium.components.messages.MessageDispatcherProvider;
import org.chromium.components.messages.MessageIdentifier;
import org.chromium.components.messages.MessageScopeType;
import org.chromium.components.messages.PrimaryActionClickBehavior;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

/**
 * Manages UI effects for speedreader including hiding and showing the
 * message UI.
 */
public class BraveSpeedReaderManager extends EmptyTabObserver implements UserData {
    /** The key to access this object from a {@Tab}. */
    public static final Class<BraveSpeedReaderManager> USER_DATA_KEY =
            BraveSpeedReaderManager.class;

    private static final String TAG = "BraveSpeedreaderMgr";

    /** If the infobar was closed due to the close button. */
    private boolean mIsDismissed;

    /** The tab this manager is attached to. */
    private final Tab mTab;

    /** The supplier of MessageDispatcher to display the message. */
    private final Supplier<MessageDispatcher> mMessageDispatcherSupplier;

    /** Whether the message ui is being shown or has already been shown. */
    private boolean mMessageShown;

    /** Property Model of Reader mode message. */
    private PropertyModel mMessageModel;

    BraveSpeedReaderManager(Tab tab, Supplier<MessageDispatcher> messageDispatcherSupplier) {
        super();
        mTab = tab;
        mTab.addObserver(this);
        mMessageDispatcherSupplier = messageDispatcherSupplier;
    }

    /**
     * Create an instance of the {@link BraveSpeedReaderManager} for the provided tab.
     * @param tab The tab that will have a manager instance attached to it.
     */
    public static void createForTab(Tab tab) {
        tab.getUserDataHost().setUserData(USER_DATA_KEY,
                new BraveSpeedReaderManager(
                        tab, () -> MessageDispatcherProvider.from(tab.getWindowAndroid())));
    }

    /** Clear the status map and references to other objects. */
    @Override
    public void destroy() {}

    @Override
    public void onShown(Tab shownTab, @TabSelectionType int type) {
        // If the reader infobar was dismissed, stop here.
        if (mIsDismissed) return;

        tryShowingPrompt();
    }

    @Override
    public void onDestroyed(Tab tab) {
        if (tab == null) return;

        removeTabState();
    }

    @Override
    public void onActivityAttachmentChanged(Tab tab, @Nullable WindowAndroid window) {
        // Intentionally do nothing to prevent automatic observer removal on detachment.
    }

    /** Clear the reader mode state for this manager. */
    private void removeTabState() {
        mIsDismissed = false;
    }

    /** Try showing the reader mode prompt. */
    void tryShowingPrompt() {
        if (!isEnabled()) return;
        if (mTab == null || mTab.getWebContents() == null) return;

        if (mIsDismissed || !BraveSpeedReaderUtils.tabSupportsDistillation(mTab)) {
            return;
        }

        MessageDispatcher messageDispatcher = mMessageDispatcherSupplier.get();
        if (messageDispatcher != null) {
            // Speedreader message is only shown once per tab.
            if (mMessageShown) {
                return;
            }
            showReaderModeMessage(messageDispatcher);
            mMessageShown = true;
        }
    }

    private void showReaderModeMessage(MessageDispatcher messageDispatcher) {
        if (mMessageModel != null) {
            // It is safe to dismiss a message which has been dismissed previously.
            messageDispatcher.dismissMessage(mMessageModel, DismissReason.DISMISSED_BY_FEATURE);
        }
        Resources resources = mTab.getContext().getResources();
        // Save url for #onMessageDismissed. Url may have been changed and became
        // different from the url when message is enqueued.
        GURL url = mTab.getUrl();
        mMessageModel = new PropertyModel.Builder(MessageBannerProperties.ALL_KEYS)
                                .with(MessageBannerProperties.MESSAGE_IDENTIFIER,
                                        MessageIdentifier.READER_MODE)
                                .with(MessageBannerProperties.TITLE,
                                        resources.getString(R.string.speedreader_message_title))
                                .with(MessageBannerProperties.PRIMARY_BUTTON_TEXT,
                                        resources.getString(R.string.speedreader_message_button))
                                .with(MessageBannerProperties.ON_PRIMARY_ACTION,
                                        () -> {
                                            BraveSpeedReaderUtils.enableSpeedreaderMode(mTab);
                                            return PrimaryActionClickBehavior.DISMISS_IMMEDIATELY;
                                        })
                                .with(MessageBannerProperties.ON_DISMISSED,
                                        (reason) -> onMessageDismissed(url, reason))
                                .build();
        messageDispatcher.enqueueMessage(
                mMessageModel, mTab.getWebContents(), MessageScopeType.NAVIGATION, false);
    }

    private void onMessageDismissed(GURL unused_url, @DismissReason int dismissReason) {
        mMessageModel = null;
        if (dismissReason == DismissReason.GESTURE) {
            mIsDismissed = true;
        }

        if (dismissReason != DismissReason.PRIMARY_ACTION) {
            mIsDismissed = false;
        }
    }

    /** @return Whether speedreader is enabled. */
    public static boolean isEnabled() {
        boolean isFeatureEnabled = ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SPEEDREADER);
        if (!isFeatureEnabled) return false;

        boolean isPrefEnabled =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getBoolean(BravePref.SPEEDREADER_PREF_ENABLED);
        return isPrefEnabled;
    }
}
