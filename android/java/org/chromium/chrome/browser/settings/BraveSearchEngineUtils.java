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
        // There is no point in creating service for OTR profile in advance since they change
        // It will be initialized in SearchEngineTabModelSelectorObserver when called on an OTR
        // profile
        tabModelSelector.addObserver(new SearchEngineTabModelSelectorObserver());

        // For first-run initialization, it needs default TemplateUrl,
        // so do it after TemplateUrlService is loaded to get it if it isn't loaded yet.
        // Init on regular profile only, leave the rest to listener, since
        // user shouldn't be able to go directly into a private tab on first run.
        final Profile profile = Profile.getLastUsedRegularProfile();
        initializeBraveSearchEngineStates(profile);
    }

    static public void initializeBraveSearchEngineStates(Profile profile) {
        TemplateUrlService templateUrlService =
                BraveTemplateUrlServiceFactory.getForProfile(profile);
        if (!templateUrlService.isLoaded()) {
            templateUrlService.registerLoadListener(new TemplateUrlService.LoadListener() {
                @Override
                public void onTemplateUrlServiceLoaded() {
                    BraveTemplateUrlServiceFactory.getForProfile(profile).unregisterLoadListener(
                            this);
                    doInitializeBraveSearchEngineStates(profile);
                }
            });
            templateUrlService.load();
        } else {
            doInitializeBraveSearchEngineStates(profile);
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
        QuickActionSearchAndBookmarkWidgetProvider.initializeDelegate();
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
