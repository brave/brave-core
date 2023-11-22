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
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.history_clusters.HistoryClustersProcessor.OpenHistoryClustersDelegate;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabWindowManager;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.PropertyModel;

class BraveAutocompleteMediator extends AutocompleteMediator implements BraveSuggestionHost {
    private static final String AUTOCOMPLETE_ENABLED = "brave.autocomplete_enabled";

    private Context mContext;
    private AutocompleteDelegate mDelegate;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private boolean mNativeInitialized;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private DropdownItemViewInfoListManager mDropdownViewInfoListManager;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private DropdownItemViewInfoListBuilder mDropdownViewInfoListBuilder;

    public BraveAutocompleteMediator(@NonNull Context context,
            @NonNull AutocompleteControllerProvider controllerProvider,
            @NonNull AutocompleteDelegate delegate,
            @NonNull UrlBarEditingTextStateProvider textProvider,
            @NonNull PropertyModel listPropertyModel, @NonNull Handler handler,
            @NonNull Supplier<ModalDialogManager> modalDialogManagerSupplier,
            @NonNull Supplier<Tab> activityTabSupplier,
            @Nullable Supplier<ShareDelegate> shareDelegateSupplier,
            @NonNull LocationBarDataProvider locationBarDataProvider,
            @NonNull Callback<Tab> bringTabToFrontCallback,
            @NonNull Supplier<TabWindowManager> tabWindowManagerSupplier,
            @NonNull BookmarkState bookmarkState,
            @NonNull OmniboxActionDelegate omniboxActionDelegate,
            @NonNull OpenHistoryClustersDelegate openHistoryClustersDelegate) {
        super(context, controllerProvider, delegate, textProvider, listPropertyModel, handler,
                modalDialogManagerSupplier, activityTabSupplier, shareDelegateSupplier,
                locationBarDataProvider, bringTabToFrontCallback, tabWindowManagerSupplier,
                bookmarkState, omniboxActionDelegate, openHistoryClustersDelegate);
        mContext = context;
        mDelegate = delegate;
    }

    @Override
    public void onTextChanged(String textWithoutAutocomplete) {
        if (ProfileManager.isInitialized()
                && !UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(AUTOCOMPLETE_ENABLED)) {
            return;
        }

        super.onTextChanged(textWithoutAutocomplete);
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
        }
        super.initDefaultProcessors();
    }
}
