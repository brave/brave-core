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
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;
                  import org.chromium.base.Log;
import java.util.List;

public class BraveSearchEngineAdapter extends SearchEngineAdapter {
    private static final String TAG = "BraveSearchEngineAdapter";

    public static final String PRIVATE_DSE_SHORTNAME = "private_dse_shortname";

    private boolean mIsPrivate;

    static public void setDSEPrefs(TemplateUrl templateUrl, boolean isPrivate) {
Log.e("TAG", "BraveSearchEngineAdapter.setDSEPrefs 000 templateUrl.getShortName()="+templateUrl.getShortName());
Log.e("TAG", "BraveSearchEngineAdapter.setDSEPrefs 000 isPrivate="+isPrivate);
        if (isPrivate) {
Log.e("TAG", "BraveSearchEngineAdapter.setDSEPrefs 001 PRIVATE will write to SharedPreferences PRIVATE_DSE_SHORTNAME: "+templateUrl.getShortName());
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putString(PRIVATE_DSE_SHORTNAME, templateUrl.getShortName());
            sharedPreferencesEditor.apply();
            
            ;
            BraveSearchEnginePrefHelper.getInstance().setPrivateSEGuid(templateUrl.getGUID() ); /*GUID*/
        } else {
            // For the regular tab we save DSE in native code
            String keyword = templateUrl.getKeyword();
Log.e("TAG", "BraveSearchEngineAdapter.setDSEPrefs 002 NORMAL will write to TemplateUrlService keyword="+keyword);
            TemplateUrlServiceFactory.get().setSearchEngine(keyword);
        }
Log.e("TAG", "BraveSearchEngineAdapter.setDSEPrefs 003 EXIT");
    }

    static public void updateActiveDSE(boolean isPrivate) {
Log.e("TAG", "BraveSearchEngineAdapter.updateActiveDSE 000 isPrivate="+isPrivate);
Log.e("TAG", "BraveSearchEngineAdapter.updateActiveDSE 000 EXIT");
return;
        // // For the regular tab, trust the native Chromium's TemplateUrlService and
        // // don't overwrite with our value to make sync work
        // if (isPrivate == false) {
        //     return;
        // }
        // 
        // TemplateUrl templateUrl = getTemplateUrlByShortName(getDSEShortName(isPrivate));
        // if (templateUrl == null) {
        //     return;
        // }
        // String keyword = templateUrl.getKeyword();
        // TemplateUrlServiceFactory.get().setSearchEngine(keyword);
    }

    static public String getDSEShortName(boolean isPrivate) {
Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 000 isPrivate="+isPrivate);
        String defaultSearchEngineName = null;
        TemplateUrl dseTemplateUrl =
                TemplateUrlServiceFactory.get().getDefaultSearchEngineTemplateUrl();
        if (dseTemplateUrl != null) defaultSearchEngineName = dseTemplateUrl.getShortName();

Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 001 defaultSearchEngineName="+defaultSearchEngineName);
//TemplateUrlServiceFactory.get().setSearchEngine
        if (isPrivate == false) {
Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 002 return defaultSearchEngineName="+defaultSearchEngineName);
            // For the regular tab, rely on the value from the native Chromium's
            // TemplateUrlService to make sync work
            return defaultSearchEngineName;
        }
Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 003 getFetchSEFromNative()="+BraveSearchEnginePrefHelper.getInstance().getFetchSEFromNative());

        // TODO(sergz): A check, do we need to fetch a default SE from native and avoid
        // overwrite.
        if (BraveSearchEnginePrefHelper.getInstance().getFetchSEFromNative()) {
Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 004 will call setDSEPrefs(false)");
            // Set it for normal tab only
            setDSEPrefs(dseTemplateUrl, false);
            BraveSearchEnginePrefHelper.getInstance().setFetchSEFromNative(false);
        }
        
        String ret = ContextUtils.getAppSharedPreferences().getString(
                PRIVATE_DSE_SHORTNAME, defaultSearchEngineName);
Log.e("TAG", "BraveSearchEngineAdapter.getDSEShortName 005 EXIT ret="+ret);
        return ret;

        // return ContextUtils.getAppSharedPreferences().getString(
        //         PRIVATE_DSE_SHORTNAME, defaultSearchEngineName);
    }

    static public TemplateUrl getTemplateUrlByShortName(String name) {
        List<TemplateUrl> templateUrls = TemplateUrlServiceFactory.get().getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
            TemplateUrl templateUrl = templateUrls.get(index);
            if (templateUrl.getShortName().equals(name)) {
                return templateUrl;
            }
        }
        assert false : "This should not happen!";
        return null;
    }

    public BraveSearchEngineAdapter(Context context, boolean isPrivate) {
        super(context);
        mIsPrivate = isPrivate;
    }

    @Override
    public void onClick(View view) {
        if (!mIsPrivate) {
            super.onClick(view);
        } else {
            int position = (int)view.getTag();
            mSelectedSearchEnginePosition = position;
            notifyDataSetChanged();
        } 

        if (view.getTag() == null) {
            return;
        }

        TemplateUrl templateUrl = (TemplateUrl) getItem((int) view.getTag());
        setDSEPrefs(templateUrl, mIsPrivate);
    }

    @Override
    public void start() {
        updateActiveDSE(mIsPrivate);
        try {
            super.start();
        } catch (IllegalStateException e) {
            // IllegalStateException indicates that search engine is not available anymore. We just
            // log an error and allow user to choose another search engine instead.
            Log.e(TAG, e.getMessage());
        }
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
