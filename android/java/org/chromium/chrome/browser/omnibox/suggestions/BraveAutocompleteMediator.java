/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;
import android.os.Handler;

import org.chromium.base.Callback;
import org.chromium.base.supplier.Supplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.brave_leo.BraveLeoUtils;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.omnibox.DeferredIMEWindowInsetApplicationCallback;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.voice.VoiceRecognitionHandler;
import org.chromium.chrome.browser.omnibox.voice.VoiceRecognitionHandler.VoiceResult;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabwindow.TabWindowManager;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.List;
import java.util.Locale;

@NullMarked
class BraveAutocompleteMediator extends AutocompleteMediator
        implements BraveSuggestionHost, BraveLeoAutocompleteDelegate {
    private static final String AUTOCOMPLETE_ENABLED = "brave.autocomplete_enabled";
    private static final String LEO_START_WORD_UPPER_CASE = "LEO";

    private final AutocompleteDelegate mDelegate;
    private final Supplier<@Nullable Tab> mActivityTabSupplier;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    @SuppressWarnings("NullAway") // Actual instance is at the parent
    private DropdownItemViewInfoListManager mDropdownViewInfoListManager;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private @Nullable DropdownItemViewInfoListBuilder mDropdownViewInfoListBuilder;

    public BraveAutocompleteMediator(
            Context context,
            AutocompleteDelegate delegate,
            UrlBarEditingTextStateProvider textProvider,
            PropertyModel listPropertyModel,
            Handler handler,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            Supplier<@Nullable Tab> activityTabSupplier,
            Supplier<ShareDelegate> shareDelegateSupplier,
            LocationBarDataProvider locationBarDataProvider,
            Callback<Tab> bringTabToFrontCallback,
            Supplier<TabWindowManager> tabWindowManagerSupplier,
            BookmarkState bookmarkState,
            OmniboxActionDelegate omniboxActionDelegate,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            OmniboxSuggestionsDropdownEmbedder embedder,
            WindowAndroid windowAndroid,
            DeferredIMEWindowInsetApplicationCallback deferredIMEWindowInsetApplicationCallback) {
        super(
                context,
                delegate,
                textProvider,
                listPropertyModel,
                handler,
                modalDialogManagerSupplier,
                activityTabSupplier,
                shareDelegateSupplier,
                locationBarDataProvider,
                bringTabToFrontCallback,
                tabWindowManagerSupplier,
                bookmarkState,
                omniboxActionDelegate,
                lifecycleDispatcher,
                embedder,
                windowAndroid,
                deferredIMEWindowInsetApplicationCallback);

        mDelegate = delegate;
        mActivityTabSupplier = activityTabSupplier;
    }

    @Override
    public void removeBraveSearchSuggestion() {
        if (mDropdownViewInfoListManager instanceof BraveDropdownItemViewInfoListManager) {
            ((BraveDropdownItemViewInfoListManager) mDropdownViewInfoListManager)
                    .removeBraveSearchSuggestion();
        }
    }

    /** We override parent to move back ability to set AutocompleteDelegate. */
    @Override
    void initDefaultProcessors() {
        if (mDropdownViewInfoListBuilder != null
                && mDropdownViewInfoListBuilder instanceof BraveDropdownItemViewInfoListBuilder) {
            ((BraveDropdownItemViewInfoListBuilder) mDropdownViewInfoListBuilder)
                    .setAutocompleteDelegate(mDelegate);
            ((BraveDropdownItemViewInfoListBuilder) mDropdownViewInfoListBuilder)
                    .setLeoAutocompleteDelegate(this);
        }
        super.initDefaultProcessors();
    }

    @Override
    public boolean isAutoCompleteEnabled(WebContents webContents) {
        if (ProfileManager.isInitialized()) {
            Profile profile = Profile.fromWebContents(webContents);
            if (profile != null && !UserPrefs.get(profile).getBoolean(AUTOCOMPLETE_ENABLED)) {
                return false;
            }
        }
        return true;
    }

    @Override
    public boolean isLeoEnabled() {
        return BraveLeoPrefUtils.isLeoEnabled();
    }

    @Override
    public void openLeoQuery(WebContents webContents, String conversationUuid, String query) {
        mDelegate.clearOmniboxFocus();
        BraveLeoUtils.openLeoQuery(webContents, conversationUuid, query, true);
    }

    @Override
    void onVoiceResults(@Nullable List<VoiceRecognitionHandler.VoiceResult> voiceResults) {
        Tab tab = mActivityTabSupplier.get();
        if (tab != null && voiceResults != null) {
            VoiceResult topResult = (voiceResults.size() > 0) ? voiceResults.get(0) : null;
            if (topResult != null) {
                String topResultQuery = topResult.getMatch();
                // Check if the query starts with the start word for Leo.
                if (topResultQuery
                        .toUpperCase(Locale.ENGLISH)
                        .startsWith(LEO_START_WORD_UPPER_CASE)) {
                    // Remove the start word from the query and process it.
                    topResultQuery =
                            topResultQuery.substring(LEO_START_WORD_UPPER_CASE.length()).trim();
                    if (tab.getWebContents() != null) {
                        openLeoQuery(tab.getWebContents(), "", topResultQuery);
                    }

                    // Clear the voice results to prevent the query from being processed by Chromium
                    // since it's already handled by Leo.
                    voiceResults.clear();
                }
            }
        }

        super.onVoiceResults(voiceResults);
    }

    // Prevents from clearing URL bar text and suggestions when
    // in multi-window/split-mode or when switch away from Brave.
    // It was introduced in that chromium commit
    // https://github.com/chromium/chromium/commit/7bcc9de3972ca4ba6feb6a136edce9d49ec30daf
    @Override
    public void onTopResumedActivityChanged(boolean isTopResumedActivity) {}
}
