/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Px;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.OmniboxPrefManager;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.styles.OmniboxImageSupplier;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.brave_leo.BraveLeoSuggestionProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerProcessor;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteResult;
import org.chromium.components.omnibox.GroupsProto.GroupConfig;
import org.chromium.components.omnibox.suggestions.OmniboxSuggestionUiType;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

class BraveDropdownItemViewInfoListBuilder extends DropdownItemViewInfoListBuilder {
    private @Nullable BraveSearchBannerProcessor mBraveSearchBannerProcessor;
    private @Nullable BraveLeoSuggestionProcessor mBraveLeoSuggestionProcessor;
    private UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private @NonNull Supplier<Tab> mActivityTabSupplier;
    private static final List<String> sBraveSearchEngineDefaultRegions =
            Arrays.asList("CA", "DE", "FR", "GB", "US", "AT", "ES", "MX");
    @Px
    private static final int DROPDOWN_HEIGHT_UNKNOWN = -1;
    private static final int DEFAULT_SIZE_OF_VISIBLE_GROUP = 5;
    private Context mContext;
    private AutocompleteDelegate mAutocompleteDelegate;
    private BraveLeoAutocompleteDelegate mLeoAutocompleteDelegate;
    private @Nullable OmniboxImageSupplier mImageSupplier;

    BraveDropdownItemViewInfoListBuilder(
            @NonNull Supplier<Tab> tabSupplier, BookmarkState bookmarkState) {
        super(tabSupplier, bookmarkState);

        mActivityTabSupplier = tabSupplier;
    }

    public void setAutocompleteDelegate(AutocompleteDelegate autocompleteDelegate) {
        mAutocompleteDelegate = autocompleteDelegate;
    }

    public void setLeoAutocompleteDelegate(BraveLeoAutocompleteDelegate leoAutocompleteDelegate) {
        mLeoAutocompleteDelegate = leoAutocompleteDelegate;
    }

    @Override
    void initDefaultProcessors(
            Context context, SuggestionHost host, UrlBarEditingTextStateProvider textProvider) {
        mContext = context;
        mUrlBarEditingTextProvider = textProvider;
        super.initDefaultProcessors(context, host, textProvider);
        if (host instanceof BraveSuggestionHost) {
            mBraveSearchBannerProcessor =
                    new BraveSearchBannerProcessor(
                            context,
                            (BraveSuggestionHost) host,
                            textProvider,
                            mAutocompleteDelegate);
            mImageSupplier = new OmniboxImageSupplier(context);
            mBraveLeoSuggestionProcessor =
                    new BraveLeoSuggestionProcessor(
                            context,
                            host,
                            textProvider,
                            mImageSupplier,
                            mLeoAutocompleteDelegate,
                            mActivityTabSupplier);
        }
    }

    @Override
    void onOmniboxSessionStateChange(boolean activated) {
        super.onOmniboxSessionStateChange(activated);
        mBraveSearchBannerProcessor.onOmniboxSessionStateChange(activated);
        mBraveLeoSuggestionProcessor.onOmniboxSessionStateChange(activated);
    }

    @Override
    void onNativeInitialized() {
        super.onNativeInitialized();
        mBraveSearchBannerProcessor.onNativeInitialized();
        mBraveLeoSuggestionProcessor.onNativeInitialized();
    }

    private int getTileNavSuggestPosition(List<DropdownItemViewInfo> viewInfoList) {
        for (int i = 0; i < viewInfoList.size(); ++i) {
            if (viewInfoList.get(i).type == OmniboxSuggestionUiType.TILE_NAVSUGGEST) {
                return i;
            }
        }
        return viewInfoList.size();
    }

    @Override
    @NonNull
    List<DropdownItemViewInfo> buildDropdownViewInfoList(AutocompleteResult autocompleteResult) {
        mBraveSearchBannerProcessor.onSuggestionsReceived();
        mBraveLeoSuggestionProcessor.onSuggestionsReceived();
        List<DropdownItemViewInfo> viewInfoList =
                super.buildDropdownViewInfoList(autocompleteResult);

        if (isBraveLeoEnabled()
                && !mUrlBarEditingTextProvider.getTextWithoutAutocomplete().isEmpty()) {
            final PropertyModel leoModel = mBraveLeoSuggestionProcessor.createModel();
            mBraveLeoSuggestionProcessor.populateModel(leoModel);
            var newMatches = autocompleteResult.getSuggestionsList();

            GroupConfig config;
            int tileNavSuggestPosition = getTileNavSuggestPosition(viewInfoList);

            // We would like to get Leo position above the most visited tiles
            // and get into the same group as the item right above, if any exists.
            // This way we will have a rounded corners around all the group
            // including the suggestion
            if (tileNavSuggestPosition > 0) {
                DropdownItemViewInfo itemAbove = viewInfoList.get(tileNavSuggestPosition - 1);
                config = itemAbove.groupConfig;
            } else {
                // There is no any item above nav suggest tiles, so use the default
                config = GroupConfig.getDefaultInstance();
            }

            viewInfoList.add(
                    tileNavSuggestPosition,
                    new DropdownItemViewInfo(mBraveLeoSuggestionProcessor, leoModel, config));
        }
        if (isBraveSearchPromoBanner()) {
            final PropertyModel model = mBraveSearchBannerProcessor.createModel();
            mBraveSearchBannerProcessor.populateModel(model);
            viewInfoList.add(new DropdownItemViewInfo(
                    mBraveSearchBannerProcessor, model, GroupConfig.getDefaultInstance()));
        }

        return viewInfoList;
    }

    private boolean isBraveLeoEnabled() {
        Tab tab = mActivityTabSupplier.get();
        if (mLeoAutocompleteDelegate != null
                && ChromeFeatureList.isEnabled(BraveFeatureList.AI_CHAT)
                && tab != null
                && !tab.isIncognito()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_LEO_AUTOCOMPLETE, true)) {
            return true;
        }

        return false;
    }

    private boolean isBraveSearchPromoBanner() {
        Tab activeTab = mActivityTabSupplier.get();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SEARCH_OMNIBOX_BANNER)
                && mUrlBarEditingTextProvider != null
                && mUrlBarEditingTextProvider.getTextWithoutAutocomplete().length() > 0
                && activeTab != null
                && !activeTab.isIncognito()
                && sBraveSearchEngineDefaultRegions.contains(Locale.getDefault().getCountry())
                && !BraveSearchEngineAdapter.getDSEShortName(
                                Profile.fromWebContents(activeTab.getWebContents()), false, null)
                        .equals("Brave")
                && !OmniboxPrefManager.getInstance().isBraveSearchPromoBannerDismissed()
                && !OmniboxPrefManager.getInstance()
                        .isBraveSearchPromoBannerDismissedCurrentSession()) {
            long expiredDate =
                    OmniboxPrefManager.getInstance().getBraveSearchPromoBannerExpiredDate();

            if (expiredDate == 0) {
                OmniboxPrefManager.getInstance().setBraveSearchPromoBannerExpiredDate();
                return true;
            } else if (expiredDate > System.currentTimeMillis()) {
                return true;
            }
            return false;
        } else {
            return false;
        }
    }
}
