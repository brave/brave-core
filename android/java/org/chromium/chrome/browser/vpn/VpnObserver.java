/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.vpn;

public interface VpnObserver {
    default public void OnGetAllServerRegions(String jsonServerRegions, boolean isSuccess){};
    default public void OnGetTimezonesForRegions(String jsonTimezones, boolean isSuccess){};
    default public void OnGetHostnamesForRegion(String jsonHostnames, boolean isSuccess){};
    default public void OnGetSubscriberCredential(String subscriberCredential, boolean isSuccess){};
    default public void OnVerifyPurchaseToken(String jsonResponse, boolean isSuccess){};
}
