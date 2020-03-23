/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.components.search_engines.TemplateUrl;

import java.util.List;

public class BraveSearchEngineUtils {
    static public String getDSEShortName(boolean isPrivate) {
        String defaultSearchEngineName = null;
        TemplateUrl dseTemplateUrl =
                TemplateUrlServiceFactory.get().getDefaultSearchEngineTemplateUrl();
        if (dseTemplateUrl != null)
            defaultSearchEngineName = dseTemplateUrl.getShortName();

        return ContextUtils.getAppSharedPreferences().getString(
            isPrivate ? BraveHelper.PRIVATE_DSE_SHORTNAME :
                BraveHelper.STANDARD_DSE_SHORTNAME, defaultSearchEngineName);
    }

    static public void updateActiveDSE(boolean isPrivate) {
        TemplateUrl templateUrl = getTemplateUrlByShortName(getDSEShortName(isPrivate));
        if (templateUrl == null) {
            return;
        }
        String keyword = templateUrl.getKeyword();
        TemplateUrlServiceFactory.get().setSearchEngine(keyword);
    }

    static public void setDSEPrefs(TemplateUrl templateUrl, boolean isPrivate) {
        SharedPreferences.Editor sharedPreferencesEditor =
            ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putString(
            isPrivate ? BraveHelper.PRIVATE_DSE_SHORTNAME :
                BraveHelper.STANDARD_DSE_SHORTNAME, templateUrl.getShortName());
        sharedPreferencesEditor.apply();
    }

    static public void initializeBraveSearchEngineStates(TabModelSelector tabModelSelector) {
        tabModelSelector.addObserver(new SearchEngineTabModelSelectorObserver(tabModelSelector));

        // For first-run initialization, it needs default TemplateUrl.
        // So, doing it after TemplateUrlService is loaded to get it if it isn't loaded yet.
        if (TemplateUrlServiceFactory.get().isLoaded())  {
            doInitializeBraveSearchEngineStates();
            return;
        }

        TemplateUrlServiceFactory.get().registerLoadListener(
            new TemplateUrlService.LoadListener() {
                @Override
                public void onTemplateUrlServiceLoaded() {
                    TemplateUrlServiceFactory.get().unregisterLoadListener(this);
                    doInitializeBraveSearchEngineStates();
                }
            });
    }

    static private void initializeDSEPrefs() {
        // At first run, we should set initial default prefs to each standard/private DSE prefs.
        // Those pref values will be used until user change DES options explicitly.
        final String notInitialized = "notInitialized";
        if (notInitialized.equals(
                ContextUtils.getAppSharedPreferences().getString(
                    BraveHelper.STANDARD_DSE_SHORTNAME, notInitialized))) {
            TemplateUrl templateUrl =
                TemplateUrlServiceFactory.get().getDefaultSearchEngineTemplateUrl();

            SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putString(BraveHelper.STANDARD_DSE_SHORTNAME,
                templateUrl.getShortName());
            sharedPreferencesEditor.putString(BraveHelper.PRIVATE_DSE_SHORTNAME,
                templateUrl.getShortName());
            sharedPreferencesEditor.apply();
        }
    }

    static private void doInitializeBraveSearchEngineStates() {
        assert TemplateUrlServiceFactory.get().isLoaded();

        initializeDSEPrefs();
        // Initially set standard dse as an active DSE.
        updateActiveDSE(false);
    }

    static public TemplateUrl getTemplateUrlByShortName(String name) {
        List<TemplateUrl> templateUrls = TemplateUrlServiceFactory.get().getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
          TemplateUrl templateUrl = templateUrls.get(index);
          if (templateUrl.getShortName().equals(name)) {
              return templateUrl;
          }
        }
        // This should not happen
        assert false;
        return null;
    }
}
