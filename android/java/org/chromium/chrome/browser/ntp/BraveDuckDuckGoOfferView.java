/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.components.search_engines.TemplateUrl;

import java.util.List;

public class BraveDuckDuckGoOfferView extends LinearLayout {
    public static String DDG_SEARCH_ENGINE_SHORT_NAME = "DuckDuckGo";
    public static String PREF_DDG_OFFER_SHOWN = "brave_ddg_offer_shown";

    private Context mContext;
    private View mDDGOfferLink;

    public BraveDuckDuckGoOfferView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mDDGOfferLink = findViewById(R.id.ddg_offer_link);
        mDDGOfferLink.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                showDDGOffer(true);
            }
        });

        showDDGOffer(false);
    }

    private void showDDGOffer(boolean forceShow) {
        if (BraveSearchEngineUtils.getDSEShortName(true).equals(DDG_SEARCH_ENGINE_SHORT_NAME)) {
            mDDGOfferLink.setVisibility(View.GONE);
            return;
        }
        if (!forceShow
                && ContextUtils.getAppSharedPreferences().getBoolean(PREF_DDG_OFFER_SHOWN, false)) {
            return;
        }
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(PREF_DDG_OFFER_SHOWN, true)
                .apply();
        new AlertDialog.Builder(mContext, R.style.BraveDialogTheme)
                .setView(R.layout.ddg_offer_layout)
                .setPositiveButton(R.string.ddg_offer_positive,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                TemplateUrl templateUrl =
                                        getTemplateUrlByShortName(DDG_SEARCH_ENGINE_SHORT_NAME);
                                if (templateUrl != null) {
                                    BraveSearchEngineUtils.setDSEPrefs(templateUrl, true);
                                    BraveSearchEngineUtils.updateActiveDSE(true);
                                }
                                mDDGOfferLink.setVisibility(View.GONE);
                            }
                        })
                .setNegativeButton(R.string.ddg_offer_negative, null)
                .show();
    }

    private TemplateUrl getTemplateUrlByShortName(String name) {
        List<TemplateUrl> templateUrls = TemplateUrlServiceFactory.get().getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
          TemplateUrl templateUrl = templateUrls.get(index);
          if (templateUrl.getShortName().equals(name)) {
              return templateUrl;
          }
        }
        return null;
    }
}
