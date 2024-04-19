/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;
import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.chrome.browser.brave_leo.BraveLeoUtils;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabWindowManager;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.PropertyModel;

class BraveAutocompleteMediator extends AutocompleteMediator
        implements BraveSuggestionHost, BraveLeoAutocompleteDelegate {
    private static final String AUTOCOMPLETE_ENABLED = "brave.autocomplete_enabled";

    private Context mContext;
    private AutocompleteDelegate mDelegate;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private boolean mNativeInitialized;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private DropdownItemViewInfoListManager mDropdownViewInfoListManager;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private DropdownItemViewInfoListBuilder mDropdownViewInfoListBuilder;

    public BraveAutocompleteMediator(
            @NonNull Context context,
            @NonNull AutocompleteDelegate delegate,
            @NonNull UrlBarEditingTextStateProvider textProvider,
            @NonNull PropertyModel listPropertyModel,
            @NonNull Handler handler,
            @NonNull Supplier<ModalDialogManager> modalDialogManagerSupplier,
            @NonNull Supplier<Tab> activityTabSupplier,
            @Nullable Supplier<ShareDelegate> shareDelegateSupplier,
            @NonNull LocationBarDataProvider locationBarDataProvider,
            @NonNull Callback<Tab> bringTabToFrontCallback,
            @NonNull Supplier<TabWindowManager> tabWindowManagerSupplier,
            @NonNull BookmarkState bookmarkState,
            @NonNull OmniboxActionDelegate omniboxActionDelegate,
            @NonNull ActivityLifecycleDispatcher lifecycleDispatcher,
            WindowAndroid windowAndroid) {
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
                windowAndroid);

        mContext = context;
        mDelegate = delegate;
    }

    @Override
    public void onOmniboxSessionStateChange(boolean activated) {
        if (!mNativeInitialized) return;

        super.onOmniboxSessionStateChange(activated);
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
        if (mDropdownViewInfoListBuilder instanceof BraveDropdownItemViewInfoListBuilder) {
            ((BraveDropdownItemViewInfoListBuilder) mDropdownViewInfoListBuilder)
                    .setAutocompleteDelegate(mDelegate);
            if (BraveConfig.AI_CHAT_ENABLED) {
                ((BraveDropdownItemViewInfoListBuilder) mDropdownViewInfoListBuilder)
                        .setLeoAutocompleteDelegate(this);
            }
        }
        super.initDefaultProcessors();
    }

    @Override
    public boolean isAutoCompleteEnabled(WebContents webContents) {
        if (ProfileManager.isInitialized()
                && !UserPrefs.get(Profile.fromWebContents(webContents))
                        .getBoolean(AUTOCOMPLETE_ENABLED)) {
            return false;
        }

        return true;
    }

    @Override
    public boolean isLeoEnabled() {
        return BraveLeoPrefUtils.isLeoEnabled();
    }

    @Override
    public void openLeoQuery(WebContents webContents, String query) {
        mDelegate.clearOmniboxFocus();
        BraveLeoUtils.openLeoQuery(webContents, query, true);
    }
}
