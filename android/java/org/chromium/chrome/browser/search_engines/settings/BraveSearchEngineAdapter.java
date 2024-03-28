/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.List;

public class BraveSearchEngineAdapter extends SearchEngineAdapter {
    private static final String TAG = "BraveSearchEngineAdapter";

    public static final String PRIVATE_DSE_SHORTNAME = "private_dse_shortname";
    public static final String STANDARD_DSE_SHORTNAME = "standard_dse_shortname";

    private Profile mProfile;
    private boolean mNeedUpdateActiveDSE;

    public BraveSearchEngineAdapter(Context context, Profile profile) {
        super(context, profile);
    }

    public static void setDSEPrefs(TemplateUrl templateUrl, Profile profile) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putString(
                profile.isOffTheRecord() ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                templateUrl.getShortName());
        sharedPreferencesEditor.apply();
    }

    public static void updateActiveDSE(Profile profile, TemplateUrlService templateUrlServiceArg) {
        String shortName = getDSEShortName(profile, false, templateUrlServiceArg);
        TemplateUrl templateUrl =
                getTemplateUrlByShortName(profile, shortName, templateUrlServiceArg);
        if (templateUrl == null) {
            return;
        }
        String keyword = templateUrl.getKeyword();
        TemplateUrlService templateUrlService =
                templateUrlServiceArg != null
                        ? templateUrlServiceArg
                        : TemplateUrlServiceFactory.getForProfile(profile);
        if (templateUrlService != null) {
            templateUrlService.setSearchEngine(keyword);
        } else {
            setDSEPrefs(templateUrl, profile);
        }
    }

    // when readJavaPrefOnly is true, only read short names from Java preference and
    // avoid calling native methods
    public static String getDSEShortName(
            Profile profile, boolean readJavaPrefOnly, TemplateUrlService templateUrlServiceArg) {
        String defaultSearchEngineName = null;

        if (!readJavaPrefOnly) {
            final TemplateUrlService templateUrlService =
                    templateUrlServiceArg != null
                            ? templateUrlServiceArg
                            : TemplateUrlServiceFactory.getForProfile(profile);
            TemplateUrl dseTemplateUrl = null;
            if (templateUrlService != null) {
                dseTemplateUrl = templateUrlService.getDefaultSearchEngineTemplateUrl();
            }
            if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();

            // TODO(sergz): A check, do we need to fetch a default SE from native and avoid
            // overwrite.
            if (BraveSearchEnginePrefHelper.getInstance().getFetchSEFromNative()) {
                // Set it for normal tab only
                setDSEPrefs(dseTemplateUrl, ProfileManager.getLastUsedRegularProfile());
                BraveSearchEnginePrefHelper.getInstance().setFetchSEFromNative(false);
            }
        }

        return ContextUtils.getAppSharedPreferences().getString(
                profile.isOffTheRecord() ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                defaultSearchEngineName);
    }

    public static TemplateUrl getTemplateUrlByShortName(
            Profile profile, String name, TemplateUrlService templateUrlServiceArg) {
        TemplateUrlService templateUrlService =
                templateUrlServiceArg != null
                        ? templateUrlServiceArg
                        : TemplateUrlServiceFactory.getForProfile(profile);
        if (templateUrlService != null) {
            List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
            for (int index = 0; index < templateUrls.size(); ++index) {
                TemplateUrl templateUrl = templateUrls.get(index);
                if (templateUrl.getShortName().equals(name)) {
                    return templateUrl;
                }
            }
        }
        assert false : "This should not happen!";
        return null;
    }

    @Override
    public void start() {
        try {
            super.start();
            TemplateUrlService templateUrlService =
                    TemplateUrlServiceFactory.getForProfile(mProfile);
            if (templateUrlService == null || !templateUrlService.isLoaded()) {
                // updateActiveDSE needs to be delayed for private because service needs to be
                // loaded if no private tab is opened already
                mNeedUpdateActiveDSE = true;
            }
        } catch (IllegalStateException e) {
            // IllegalStateException indicates that search engine is not available anymore. We just
            // log an error and allow user to choose another search engine instead.
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    public void stop() {
        if (!mProfile.isNativeInitialized()) {
            return;
        }
        TemplateUrlService templateUrlService = TemplateUrlServiceFactory.getForProfile(mProfile);
        // For some reason there is a short period of time when native reference to the profile
        // has been destroyed but Java reference still exists. The stop() function only removes
        // listeners on the service, but since the profile is destroyed, the service is
        // destroyed too
        if (templateUrlService != null) {
            super.stop();
        }
    }

    // OnClickListener:

    @Override
    public void onClick(View view) {
        super.onClick(view);

        if (view.getTag() == null) {
            return;
        }

        TemplateUrl templateUrl = (TemplateUrl) getItem((int) view.getTag());
        setDSEPrefs(templateUrl, mProfile);
    }

    // TemplateUrlService.LoadListener

    @Override
    public void onTemplateUrlServiceLoaded() {
        // It is necessary to ensure user's selection is updated on first entering private setting
        // but it causes updateActiveDSE() to be called twice (once here and
        // once from SearchEngineTabModelSelectorObserver)
        if (mNeedUpdateActiveDSE) {
            mNeedUpdateActiveDSE = false;
            updateActiveDSE(mProfile, null);
        }
        super.onTemplateUrlServiceLoaded();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        TextView url = (TextView) view.findViewById(R.id.url);
        if (url != null) {
            url.setVisibility(View.GONE);
        }
        return view;
    }
}
