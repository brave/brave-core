/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

public class BraveVpnPrefModel {
    private String mPurchaseToken;
    private String mProductId;
    private String mSubscriberCredential;
    private String mClientId;
    private String mApiAuthToken;
    private String mHostname;
    private String mHostnameDisplay;
    private BraveVpnServerRegion mServerRegion;
    private String mClientPrivateKey;
    private String mClientPublicKey;

    public void setPurchaseToken(String purchaseToken) {
        mPurchaseToken = purchaseToken;
    }

    public void setProductId(String productId) {
        mProductId = productId;
    }

    public void setSubscriberCredential(String subscriberCredential) {
        mSubscriberCredential = subscriberCredential;
    }

    public void setClientId(String clientId) {
        mClientId = clientId;
    }

    public void setApiAuthToken(String apiAuthToken) {
        mApiAuthToken = apiAuthToken;
    }

    public void setHostname(String hostname) {
        mHostname = hostname;
    }

    public void setHostnameDisplay(String hostnameDisplay) {
        mHostnameDisplay = hostnameDisplay;
    }

    public void setServerRegion(BraveVpnServerRegion serverRegion) {
        mServerRegion = serverRegion;
    }

    public void setClientPrivateKey(String clientPrivateKey) {
        mClientPrivateKey = clientPrivateKey;
    }

    public void setClientPublicKey(String clientPublicKey) {
        mClientPublicKey = clientPublicKey;
    }

    public String getPurchaseToken() {
        return mPurchaseToken;
    }

    public String getProductId() {
        return mProductId;
    }

    public String getSubscriberCredential() {
        return mSubscriberCredential;
    }

    public String getClientId() {
        return mClientId;
    }

    public String getApiAuthToken() {
        return mApiAuthToken;
    }

    public String getHostname() {
        return mHostname;
    }

    public String getHostnameDisplay() {
        return mHostnameDisplay;
    }

    public BraveVpnServerRegion getServerRegion() {
        return mServerRegion;
    }

    public String getClientPrivateKey() {
        return mClientPrivateKey;
    }

    public String getClientPublicKey() {
        return mClientPublicKey;
    }
}
