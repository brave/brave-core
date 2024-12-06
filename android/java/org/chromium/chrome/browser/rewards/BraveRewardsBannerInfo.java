/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import androidx.annotation.VisibleForTesting;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;

/**
 * BraveRewardsNativeWorker.GetPublisherBanner json response converted to this pojo class
 * */
public class BraveRewardsBannerInfo {
    public static final String PUBLISHER_KEY = "publisher_key";
    public static final String TITLE = "title";
    public static final String NAME = "name";
    public static final String DESCRIPTION = "description";
    public static final String BACKGROUND = "background";
    public static final String LOGO = "logo";
    public static final String PROVIDER = "provider";
    public static final String LINKS = "links";
    public static final String STATUS = "status";
    public static final String WEB3_URL = "web3_url";

    private String mPublisherKey;
    private String mTitle;
    private String mName;
    private String mDescription;
    private String mBackground;

    private String mLogo;
    private String mProvider;
    private HashMap<String, String> mLinks;
    private int mStatus;
    private String mWeb3Url;

    public String getPublisherKey() {
        return mPublisherKey;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getName() {
        return mName;
    }

    public String getDescription() {
        return mDescription;
    }

    public String getBackground() {
        return mBackground;
    }

    public String getLogo() {
        return mLogo;
    }

    public String getProvider() {
        return mProvider;
    }

    public HashMap<String, String> getLinks() {
        return mLinks;
    }

    public int getStatus() {
        return mStatus;
    }

    public String getWeb3Url() {
        return mWeb3Url;
    }

    public BraveRewardsBannerInfo(String jsonExternalWallet) throws JSONException {
        fromJson(jsonExternalWallet);
    }

    private void fromJson(String jsonExternalWallet) throws JSONException {
        JSONObject jsonObj = new JSONObject(jsonExternalWallet);
        mPublisherKey = jsonObj.getString(PUBLISHER_KEY);
        mTitle = jsonObj.getString(TITLE);
        mName = jsonObj.getString(NAME);
        mDescription = jsonObj.getString(DESCRIPTION);
        mBackground = jsonObj.getString(BACKGROUND);
        mLogo = jsonObj.getString(LOGO);
        mWeb3Url = jsonObj.getString(WEB3_URL);

        mProvider = jsonObj.getString(PROVIDER);
        JSONObject linksJsonObject = jsonObj.getJSONObject(LINKS);
        if (linksJsonObject != null) {
            JSONArray array = linksJsonObject.names();
            if (array != null) {
                mLinks = new HashMap<>();
                for (int index = 0; index < array.length(); index++) {
                    mLinks.put(array.getString(index),
                            linksJsonObject.getString(array.getString(index)));
                }
            }
        }

        mStatus = jsonObj.getInt(STATUS);
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsBannerInfo{"
                + "mPublisherKey='" + mPublisherKey + '\'' + ", mTitle='" + mTitle + '\''
                + ", mName='" + mName + '\'' + ", mDescription='" + mDescription + '\''
                + ", mBackground='" + mBackground + '\'' + ", mLogo='" + mLogo + '\''
                + ", mProvider='" + mProvider + '\'' + ", mLinks=" + mLinks + ", mStatus=" + mStatus
                + '}';
    }
}
