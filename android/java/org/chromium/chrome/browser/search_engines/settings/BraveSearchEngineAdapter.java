/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.BraveTemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.components.search_engines.TemplateUrl;

import java.util.List;

public class BraveSearchEngineAdapter extends SearchEngineAdapter {
    private static final String TAG = "BraveSearchEngineAdapter";

    public static final String PRIVATE_DSE_SHORTNAME = "private_dse_shortname";
    public static final String STANDARD_DSE_SHORTNAME = "standard_dse_shortname";

    private boolean mIsPrivate;
    private Profile mProfile;
    private boolean needUpdateActiveDSE;

    public BraveSearchEngineAdapter(Context context, boolean isPrivate) {
        super(context);
        mIsPrivate = isPrivate;

        // Only need last used profile because we are in settings
        if (mProfile == null) {
            if (!mIsPrivate) {
                mProfile = Profile.getLastUsedRegularProfile();
            } else {
                mProfile = Profile.getLastUsedRegularProfile().getPrimaryOTRProfile(
                        /* createIfNeeded= */ true);
            }
        }
    }

    static public void setDSEPrefs(TemplateUrl templateUrl, Profile profile) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putString(
                profile.isOffTheRecord() ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                templateUrl.getShortName());
        sharedPreferencesEditor.apply();
    }

    static public void updateActiveDSE(Profile profile) {
        String shortName = getDSEShortName(profile, false);
        TemplateUrl templateUrl = getTemplateUrlByShortName(profile, shortName);
        if (templateUrl == null) {
            return;
        }
        String keyword = templateUrl.getKeyword();
        BraveTemplateUrlServiceFactory.getForProfile(profile).setSearchEngine(keyword);
    }

    // when readJavaPrefOnly is true, only read short names from Java preference and
    // avoid calling native methods
    static public String getDSEShortName(Profile profile, boolean readJavaPrefOnly) {
        String defaultSearchEngineName = null;

        if (!readJavaPrefOnly) {
            TemplateUrl dseTemplateUrl = BraveTemplateUrlServiceFactory.getForProfile(profile)
                                                 .getDefaultSearchEngineTemplateUrl();
            if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();

            // TODO(sergz): A check, do we need to fetch a default SE from native and avoid
            // overwrite.
            if (BraveSearchEnginePrefHelper.getInstance().getFetchSEFromNative()) {
                // Set it for normal tab only
                setDSEPrefs(dseTemplateUrl, Profile.getLastUsedRegularProfile());
                BraveSearchEnginePrefHelper.getInstance().setFetchSEFromNative(false);
            }
        }

        return ContextUtils.getAppSharedPreferences().getString(
                profile.isOffTheRecord() ? PRIVATE_DSE_SHORTNAME : STANDARD_DSE_SHORTNAME,
                defaultSearchEngineName);
    }

    static public TemplateUrl getTemplateUrlByShortName(Profile profile, String name) {
        List<TemplateUrl> templateUrls =
                BraveTemplateUrlServiceFactory.getForProfile(profile).getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
            TemplateUrl templateUrl = templateUrls.get(index);
            if (templateUrl.getShortName().equals(name)) {
                return templateUrl;
            }
        }
        assert false : "This should not happen!";
        return null;
    }

    @Override
    public void start() {
        try {
            runTemplateUrlServiceWithProfile(() -> {
                super.start();
                if (!BraveTemplateUrlServiceFactory.getForProfile(mProfile).isLoaded()) {
                    // updateActiveDSE needs to be delayed for private because service needs to be
                    // loaded if no private tab is opened already
                    needUpdateActiveDSE = true;
                }
            });
        } catch (IllegalStateException e) {
            // IllegalStateException indicates that search engine is not available anymore. We just
            // log an error and allow user to choose another search engine instead.
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    public void stop() {
        runTemplateUrlServiceWithProfile(() -> { super.stop(); });
    }

    // OnClickListener:

    @Override
    public void onClick(View view) {
        runTemplateUrlServiceWithProfile(() -> { super.onClick(view); });

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
        if (needUpdateActiveDSE) {
            needUpdateActiveDSE = false;
            updateActiveDSE(mProfile);
        }
        runTemplateUrlServiceWithProfile(() -> { super.onTemplateUrlServiceLoaded(); });
    }

    @Override
    public void onTemplateURLServiceChanged() {
        runTemplateUrlServiceWithProfile(() -> { super.onTemplateURLServiceChanged(); });
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

    // Wrapper for setting profile when running BraveTemplateUrlServiceFactory.get()
    // No need to synchronize because BraveTemplateUrlServiceFactory shall always be on main UI
    // thread
    private void runTemplateUrlServiceWithProfile(Runnable r) {
        BraveTemplateUrlServiceFactory.setCurrentProfile(mProfile);
        r.run();
        BraveTemplateUrlServiceFactory.setCurrentProfile(null);
    }
}
