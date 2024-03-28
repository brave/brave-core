/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

public class BraveSearchEngineUtils {
    public static void initializeBraveSearchEngineStates(TabModelSelector tabModelSelector) {
        // There is no point in creating service for OTR profile in advance since they change
        // It will be initialized in SearchEngineTabModelSelectorObserver when called on an OTR
        // profile
        tabModelSelector.addObserver(new SearchEngineTabModelSelectorObserver());

        // For first-run initialization, it needs default TemplateUrl,
        // so do it after TemplateUrlService is loaded to get it if it isn't loaded yet.
        // Init on regular profile only, leave the rest to listener, since
        // user shouldn't be able to go directly into a private tab on first run.
        final Profile profile = ProfileManager.getLastUsedRegularProfile();
        initializeBraveSearchEngineStates(profile);
    }

    public static void initializeBraveSearchEngineStates(Profile profile) {
        final TemplateUrlService templateUrlService =
                TemplateUrlServiceFactory.getForProfile(profile);

        if (!templateUrlService.isLoaded()) {
            templateUrlService.registerLoadListener(
                    new TemplateUrlService.LoadListener() {
                        @Override
                        public void onTemplateUrlServiceLoaded() {
                            templateUrlService.unregisterLoadListener(this);
                            doInitializeBraveSearchEngineStates(profile, null);
                        }
                    });
            templateUrlService.load();
        } else {
            doInitializeBraveSearchEngineStates(profile, templateUrlService);
        }
    }

    private static void initializeDSEPrefs(Profile profile) {
        // At first run, we should set initial default prefs to each standard/private DSE prefs.
        // Those pref values will be used until user change DES options explicitly.
        final String notInitialized = "notInitialized";
        if (notInitialized.equals(ContextUtils.getAppSharedPreferences().getString(
                    BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, notInitialized))) {
            final TemplateUrlService templateUrlService =
                    TemplateUrlServiceFactory.getForProfile(profile);

            TemplateUrl templateUrl = templateUrlService.getDefaultSearchEngineTemplateUrl();
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putString(
                    BraveSearchEngineAdapter.STANDARD_DSE_SHORTNAME, templateUrl.getShortName());
            sharedPreferencesEditor.putString(
                    BraveSearchEngineAdapter.PRIVATE_DSE_SHORTNAME, templateUrl.getShortName());
            sharedPreferencesEditor.apply();
        }
    }

    private static void doInitializeBraveSearchEngineStates(
            Profile profile, TemplateUrlService templateUrlServiceArg) {
        final TemplateUrlService templateUrlService =
                templateUrlServiceArg != null
                        ? templateUrlServiceArg
                        : TemplateUrlServiceFactory.getForProfile(profile);
        assert templateUrlService.isLoaded();

        initializeDSEPrefs(profile);
        updateActiveDSE(profile, templateUrlService);
        QuickActionSearchAndBookmarkWidgetProvider.initializeDelegate();
    }

    public static void setDSEPrefs(TemplateUrl templateUrl, Profile profile) {
        BraveSearchEngineAdapter.setDSEPrefs(templateUrl, profile);
        if (!profile.isOffTheRecord() && templateUrl != null) {
            QuickActionSearchAndBookmarkWidgetProvider.updateSearchEngine(
                    templateUrl.getShortName());
        }
    }

    public static void updateActiveDSE(Profile profile, TemplateUrlService templateUrlServiceArg) {
        BraveSearchEngineAdapter.updateActiveDSE(profile, templateUrlServiceArg);
    }

    public static String getDSEShortName(Profile profile, boolean javaOnly) {
        return BraveSearchEngineAdapter.getDSEShortName(profile, javaOnly, null);
    }

    public static TemplateUrl getTemplateUrlByShortName(Profile profile, String name) {
        return BraveSearchEngineAdapter.getTemplateUrlByShortName(profile, name, null);
    }
}
