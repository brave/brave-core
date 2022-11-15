/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.BraveTemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

public class BraveSearchEngineUtils {
    static public void initializeBraveSearchEngineStates(TabModelSelector tabModelSelector) {
        tabModelSelector.addObserver(new SearchEngineTabModelSelectorObserver(tabModelSelector));

        // For first-run initialization, it needs default TemplateUrl.
        // So, doing it after TemplateUrlService is loaded to get it if it isn't loaded yet.
        // Init on current (regular) profile only, leave the rest to observer, since
        // user shouldn't be able to go directly into a private tab on first run.
        Profile profile = tabModelSelector.getCurrentModel().getProfile();
        if (profile == null) profile = Profile.getLastUsedRegularProfile().getOriginalProfile();
        if (BraveTemplateUrlServiceFactory.getForProfile(profile.getOriginalProfile()).isLoaded()) {
            doInitializeBraveSearchEngineStates(profile.getOriginalProfile());
            return;
        }

        // Regular profile
        final Profile regularProfile = profile.getOriginalProfile();
        BraveTemplateUrlServiceFactory.getForProfile(regularProfile)
                .registerLoadListener(new TemplateUrlService.LoadListener() {
                    @Override
                    public void onTemplateUrlServiceLoaded() {
                        BraveTemplateUrlServiceFactory.getForProfile(regularProfile)
                                .unregisterLoadListener(this);
                        doInitializeBraveSearchEngineStates(regularProfile);
                    }
                });
    }

    // There is no point in creating service for OTR profile in advance since they change
    // This is for SearchEngineTabModelSelectorObserver when called on an OTR profile
    static public void initializePrivateBraveSearchEngineStates(Profile oTRProfile) {
        TemplateUrlService templateUrlService =
                BraveTemplateUrlServiceFactory.getForProfile(oTRProfile);
        if (!templateUrlService.isLoaded()) {
            templateUrlService.registerLoadListener(new TemplateUrlService.LoadListener() {
                @Override
                public void onTemplateUrlServiceLoaded() {
                    BraveTemplateUrlServiceFactory.getForProfile(oTRProfile)
                            .unregisterLoadListener(this);
                    doInitializeBraveSearchEngineStates(oTRProfile);
                }
            });
            templateUrlService.load();
        } else {
            doInitializeBraveSearchEngineStates(oTRProfile);
        }
    }

    static private void initializeDSEPrefs(Profile profile) {
        // At first run, we should set initial default prefs to each standard/private DSE prefs.
        // Those pref values will be used until user change DES options explicitly.
        final String notInitialized = "notInitialized";
        if (notInitialized.equals(ContextUtils.getAppSharedPreferences().getString(
                    BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, notInitialized))) {
            TemplateUrl templateUrl = BraveTemplateUrlServiceFactory.getForProfile(profile)
                                              .getDefaultSearchEngineTemplateUrl();

            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putString(
                    BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, templateUrl.getShortName());
            sharedPreferencesEditor.putString(
                    BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME, templateUrl.getShortName());
            sharedPreferencesEditor.apply();
        }
    }

    static private void doInitializeBraveSearchEngineStates(Profile profile) {
        assert BraveTemplateUrlServiceFactory.getForProfile(profile).isLoaded();

        initializeDSEPrefs(profile);
        updateActiveDSE(profile);
    }

    static public void setDSEPrefs(TemplateUrl templateUrl, Profile profile) {
        BraveSearchEngineAdapter.setDSEPrefs(templateUrl, profile);
        if (!profile.isOffTheRecord() && templateUrl != null) {
            QuickActionSearchAndBookmarkWidgetProvider.updateSearchEngine(
                    templateUrl.getShortName());
        }
    }

    static public void updateActiveDSE(Profile profile) {
        BraveSearchEngineAdapter.updateActiveDSE(profile);
    }

    static public String getDSEShortName(Profile profile, boolean javaOnly) {
        return BraveSearchEngineAdapter.getDSEShortName(profile, javaOnly);
    }

    static public TemplateUrl getTemplateUrlByShortName(Profile profile, String name) {
        return BraveSearchEngineAdapter.getTemplateUrlByShortName(profile, name);
    }
}
