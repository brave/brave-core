/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider.ControlsPosition;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.OmniboxPrefManager;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.brave_leo.BraveLeoSuggestionProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerProcessor;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteInput;
import org.chromium.components.omnibox.AutocompleteResult;
import org.chromium.components.omnibox.GroupsProto.GroupConfig;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.components.omnibox.suggestions.OmniboxSuggestionUiType;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.function.Supplier;

class BraveDropdownItemViewInfoListBuilder extends DropdownItemViewInfoListBuilder {
    private @Nullable BraveSearchBannerProcessor mBraveSearchBannerProcessor;
    private @Nullable BraveLeoSuggestionProcessor mBraveLeoSuggestionProcessor;
    private UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private final @NonNull Supplier<Tab> mActivityTabSupplier;
    private static final List<String> sBraveSearchEngineDefaultRegions =
            Arrays.asList("CA", "DE", "FR", "GB", "US", "AT", "ES", "MX");
    private AutocompleteDelegate mAutocompleteDelegate;
    private BraveLeoAutocompleteDelegate mLeoAutocompleteDelegate;
    private @Nullable Profile mProfile;

    BraveDropdownItemViewInfoListBuilder(
            Supplier<@Nullable Tab> tabSupplier,
            BookmarkState bookmarkState,
            MonotonicObservableSupplier<@ControlsPosition Integer> toolbarPositionSupplier) {
        super(tabSupplier, bookmarkState, toolbarPositionSupplier);

        mActivityTabSupplier = tabSupplier;
    }

    public void setAutocompleteDelegate(AutocompleteDelegate autocompleteDelegate) {
        mAutocompleteDelegate = autocompleteDelegate;
    }

    public void setLeoAutocompleteDelegate(BraveLeoAutocompleteDelegate leoAutocompleteDelegate) {
        mLeoAutocompleteDelegate = leoAutocompleteDelegate;
    }

    @Override
    void setProfile(Profile profile) {
        super.setProfile(profile);
        mProfile = profile;
    }

    @Override
    void initDefaultProcessors(
            Context context,
            SuggestionHost host,
            UrlBarEditingTextStateProvider textProvider,
            OmniboxActionDelegate actionDelegate) {
        mUrlBarEditingTextProvider = textProvider;
        super.initDefaultProcessors(context, host, textProvider, actionDelegate);
        if (host instanceof BraveSuggestionHost) {
            mBraveSearchBannerProcessor =
                    new BraveSearchBannerProcessor(
                            context,
                            (BraveSuggestionHost) host,
                            textProvider,
                            mAutocompleteDelegate);
            AutocompleteUIContext uiContext =
                    createUIContext(context, host, textProvider, actionDelegate);
            mBraveLeoSuggestionProcessor = new BraveLeoSuggestionProcessor(uiContext);
            mBraveLeoSuggestionProcessor.setBraveLeoAutocompleteDelegate(mLeoAutocompleteDelegate);
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
    List<DropdownItemViewInfo> buildDropdownViewInfoList(
            AutocompleteInput input, AutocompleteResult autocompleteResult) {
        mBraveSearchBannerProcessor.onSuggestionsReceived();
        mBraveLeoSuggestionProcessor.onSuggestionsReceived();
        List<DropdownItemViewInfo> viewInfoList =
                super.buildDropdownViewInfoList(input, autocompleteResult);

        // We want to show Leo auto suggestion even if the whole auto complete feature
        // is disabled
        Tab tab = mActivityTabSupplier.get();
        boolean autocompleteEnabled;
        if (tab != null) {
            autocompleteEnabled =
                    mLeoAutocompleteDelegate.isAutoCompleteEnabled(tab.getWebContents());
        } else {
            // The search widget launches SearchActivity without a tab, so use the profile
            // pushed in via setProfile to honor the "Show browser suggestions" preference.
            // If the profile hasn't been pushed in yet (e.g. cold start from the widget after
            // a force-stop), suppress suggestions until we can read the pref.
            autocompleteEnabled =
                    mProfile != null
                            && UserPrefs.get(mProfile).getBoolean(BravePref.AUTOCOMPLETE_ENABLED);
        }
        if (!autocompleteEnabled && viewInfoList.size() > 0) {
            DropdownItemViewInfo firstObj = viewInfoList.get(0);
            viewInfoList.clear();
            if (firstObj.processor != null
                    && (firstObj.processor.getViewTypeId()
                                    == OmniboxSuggestionUiType.EDIT_URL_SUGGESTION
                            || firstObj.processor.getViewTypeId()
                                    == OmniboxSuggestionUiType.CLIPBOARD_SUGGESTION)) {
                viewInfoList.add(firstObj);
            }
        }

        if (isBraveLeoEnabled()
                && !mUrlBarEditingTextProvider.getTextWithoutAutocomplete().isEmpty()) {
            final PropertyModel leoModel = mBraveLeoSuggestionProcessor.createModel();
            mBraveLeoSuggestionProcessor.populateModel(leoModel);

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

            // Handle rounded corners for leo and previous item.
            if (viewInfoList.size() > 0) {
                // Leo joins the same group below the previous last item, so that
                // item must no longer round its bottom corners. Drop the bottom
                // rounding from its positional mode while keeping any top rounding.
                PropertyModel previousModel = viewInfoList.get(viewInfoList.size() - 1).model;
                @SuggestionCommonProperties.PositionalMode
                int previousMode = previousModel.get(SuggestionCommonProperties.BG_POSITIONAL_MODE);
                if (previousMode == SuggestionCommonProperties.PositionalMode.SINGLE) {
                    previousModel.set(
                            SuggestionCommonProperties.BG_POSITIONAL_MODE,
                            SuggestionCommonProperties.PositionalMode.TOP);
                } else if (previousMode == SuggestionCommonProperties.PositionalMode.BOTTOM) {
                    previousModel.set(
                            SuggestionCommonProperties.BG_POSITIONAL_MODE,
                            SuggestionCommonProperties.PositionalMode.MIDDLE);
                }
                previousModel.set(SuggestionCommonProperties.SHOW_DIVIDER, true);
            }

            // Leo is the last item of the group, and also the first when nothing
            // sits above it.
            leoModel.set(
                    SuggestionCommonProperties.BG_POSITIONAL_MODE,
                    viewInfoList.size() == 0
                            ? SuggestionCommonProperties.PositionalMode.SINGLE
                            : SuggestionCommonProperties.PositionalMode.BOTTOM);
            leoModel.set(SuggestionCommonProperties.SHOW_DIVIDER, false);

            viewInfoList.add(
                    tileNavSuggestPosition,
                    new DropdownItemViewInfo(mBraveLeoSuggestionProcessor, leoModel, config));
        }
        if (isBraveSearchPromoBanner() && autocompleteEnabled) {
            final PropertyModel model = mBraveSearchBannerProcessor.createModel();
            mBraveSearchBannerProcessor.populateModel(model);
            viewInfoList.add(
                    new DropdownItemViewInfo(
                            mBraveSearchBannerProcessor, model, GroupConfig.getDefaultInstance()));
        }

        return viewInfoList;
    }

    private boolean isBraveLeoEnabled() {
        Tab tab = mActivityTabSupplier.get();
        if (tab != null
                && !tab.isIncognito()
                && mLeoAutocompleteDelegate != null
                && mLeoAutocompleteDelegate.isLeoEnabled()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_LEO_AUTOCOMPLETE, true)) {
            return true;
        }

        return false;
    }

    private boolean isBraveSearchPromoBanner() {
        Tab activeTab = mActivityTabSupplier.get();
        if (activeTab != null
                && !activeTab.isIncognito()
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SEARCH_OMNIBOX_BANNER)
                && mUrlBarEditingTextProvider != null
                && mUrlBarEditingTextProvider.getTextWithoutAutocomplete().length() > 0
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

    @SuppressWarnings("UnusedVariable")
    private AutocompleteUIContext createUIContext(
            Context context,
            SuggestionHost host,
            UrlBarEditingTextStateProvider textProvider,
            OmniboxActionDelegate actionDelegate) {
        assert false
                : "This method will be deleted in bytecode. Method from the parent class will be"
                        + " used instead.";
        return null;
    }
}
