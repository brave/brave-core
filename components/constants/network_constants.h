/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_NETWORK_CONSTANTS_H_
#define BRAVE_COMPONENTS_CONSTANTS_NETWORK_CONSTANTS_H_

inline constexpr char kExtensionUpdaterDomain[] = "extensionupdater.brave.com";

inline constexpr char kBraveProxyPattern[] = "https://*.brave.com/*";
inline constexpr char kBraveSoftwareProxyPattern[] =
    "https://*.bravesoftware.com/*";

inline constexpr char kBraveUsageStandardPath[] = "/1/usage/brave-core";

inline constexpr char kBraveReferralsInitPath[] = "/promo/initialize/nonua";
inline constexpr char kBraveReferralsActivityPath[] = "/promo/activity";

inline constexpr char kBraveSafeBrowsing2Proxy[] = "safebrowsing2.brave.com";
inline constexpr char kBraveSafeBrowsingSslProxy[] = "sb-ssl.brave.com";
inline constexpr char kBraveRedirectorProxy[] = "redirector.brave.com";
inline constexpr char kBraveClients4Proxy[] = "clients4.brave.com";
inline constexpr char kBraveStaticProxy[] = "static1.brave.com";

inline constexpr char kAutofillPrefix[] = "https://www.gstatic.com/autofill/*";
inline constexpr char kClients4Prefix[] = "*://clients4.google.com/";
inline constexpr char kCRXDownloadPrefix[] =
    "*://clients2.googleusercontent.com/crx/blobs/*crx*";
inline constexpr char kEmptyDataURI[] = "data:text/plain,";
inline constexpr char kEmptyImageDataURI[] =
    "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///"
    "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
inline constexpr char kJSDataURLPrefix[] =
    "data:application/javascript;base64,";
inline constexpr char kGeoLocationsPattern[] =
    "https://www.googleapis.com/geolocation/v1/geolocate?key=*";
inline constexpr char kSafeBrowsingPrefix[] =
    "https://safebrowsing.googleapis.com/";
inline constexpr char kSafeBrowsingCrxListPrefix[] =
    "https://safebrowsing.google.com/safebrowsing/clientreport/crx-list-info";
inline constexpr char kSafeBrowsingFileCheckPrefix[] =
    "https://sb-ssl.google.com/safebrowsing/clientreport/download";
inline constexpr char kCRLSetPrefix1[] =
    "*://dl.google.com/release2/chrome_component/*crl-set*";
inline constexpr char kCRLSetPrefix2[] =
    "*://*.gvt1.com/edgedl/release2/chrome_component/*";
inline constexpr char kCRLSetPrefix3[] =
    "*://www.google.com/dl/release2/chrome_component/*";
inline constexpr char kCRLSetPrefix4[] =
    "*://storage.googleapis.com/update-delta/hfnkpimlhhgieaddgfemjhofmfblmnib"
    "/*crxd";
inline constexpr char kChromeCastPrefix[] =
    "*://*.gvt1.com/edgedl/chromewebstore/*pkedcjkdefgpdelpbcmbmeomcjbeemfm*";

inline constexpr char kWidevineGvt1Prefix[] =
    "*://*.gvt1.com/*oimompecagnajdejgnnjijobebaeigek*";
inline constexpr char kWidevineGoogleDlPrefix[] =
    "*://dl.google.com/*oimompecagnajdejgnnjijobebaeigek*";

inline constexpr char kUserAgentHeader[] = "User-Agent";
inline constexpr char kBraveServicesKeyHeader[] = "BraveServiceKey";

inline constexpr char kBittorrentMimeType[] = "application/x-bittorrent";
inline constexpr char kOctetStreamMimeType[] = "application/octet-stream";

inline constexpr char kSecGpcHeader[] = "Sec-GPC";

#endif  // BRAVE_COMPONENTS_CONSTANTS_NETWORK_CONSTANTS_H_
