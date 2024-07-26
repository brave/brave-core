/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn;

public interface BraveVpnObserver {
    public default void onGetAllServerRegions(String jsonServerRegions, boolean isSuccess) {}
    ;

    public default void onGetServerRegionsWithCities(String jsonServerRegions, boolean isSuccess) {}
    ;

    public default void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {}
    ;

    public default void onGetHostnamesForRegion(String jsonHostnames, boolean isSuccess) {}
    ;

    public default void onGetWireguardProfileCredentials(
            String jsonWireguardProfileCredentials, boolean isSuccess) {}
    ;

    public default void onVerifyCredentials(String jsonVerifyCredentials, boolean isSuccess) {}
    ;

    public default void onInvalidateCredentials(
            String jsonInvalidateCredentials, boolean isSuccess) {}
    ;

    public default void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {}
    ;

    public default void onVerifyPurchaseToken(
            String jsonResponse, String purchaseToken, String productId, boolean isSuccess) {}
    ;
}
