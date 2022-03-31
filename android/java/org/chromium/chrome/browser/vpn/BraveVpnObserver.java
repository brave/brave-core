/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.vpn;

public interface BraveVpnObserver {
    default public void onGetAllServerRegions(String jsonServerRegions, boolean isSuccess){};
    default public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess){};
    default public void onGetHostnamesForRegion(String jsonHostnames, boolean isSuccess){};
    default public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess){};
    default public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess){};
    default public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess){};
}
