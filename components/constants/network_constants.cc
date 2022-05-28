/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/network_constants.h"

const char kExtensionUpdaterDomain[] = "extensionupdater.brave.com";

const char kBraveProxyPattern[] = "https://*.brave.com/*";
const char kBraveSoftwareProxyPattern[] = "https://*.bravesoftware.com/*";

const char kBraveUsageStandardPath[] = "/1/usage/brave-core";
const char kBraveUsageThresholdPath[] = "/1/usage/brave-core-threshold";

const char kBraveReferralsServer[] = "laptop-updates.brave.com";
const char kBraveReferralsInitPath[] = "/promo/initialize/nonua";
const char kBraveReferralsActivityPath[] = "/promo/activity";

const char kBraveSafeBrowsing2Proxy[] = "safebrowsing2.brave.com";
const char kBraveSafeBrowsingSslProxy[] = "sb-ssl.brave.com";
const char kBraveRedirectorProxy[] = "redirector.brave.com";
const char kBraveClients4Proxy[] = "clients4.brave.com";
const char kBraveStaticProxy[] = "static1.brave.com";

const char kAutofillPrefix[] = "https://www.gstatic.com/autofill/*";
const char kClients4Prefix[] = "*://clients4.google.com/";
const char kCRXDownloadPrefix[] =
    "*://clients2.googleusercontent.com/crx/blobs/*crx*";
const char kEmptyDataURI[] = "data:text/plain,";
const char kEmptyImageDataURI[] =
    "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///"
    "yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
const char kJSDataURLPrefix[] = "data:application/javascript;base64,";
const char kGeoLocationsPattern[] =
    "https://www.googleapis.com/geolocation/v1/geolocate?key=*";
const char kSafeBrowsingPrefix[] = "https://safebrowsing.googleapis.com/";
const char kSafeBrowsingCrxListPrefix[] =
    "https://safebrowsing.google.com/safebrowsing/clientreport/crx-list-info";
const char kSafeBrowsingFileCheckPrefix[] =
    "https://sb-ssl.google.com/safebrowsing/clientreport/download";
const char kCRLSetPrefix1[] =
    "*://dl.google.com/release2/chrome_component/*crl-set*";
const char kCRLSetPrefix2[] =
    "*://*.gvt1.com/edgedl/release2/chrome_component/*";
const char kCRLSetPrefix3[] =
    "*://www.google.com/dl/release2/chrome_component/*crl-set*";
const char kCRLSetPrefix4[] =
    "*://storage.googleapis.com/update-delta/hfnkpimlhhgieaddgfemjhofmfblmnib"
    "/*crxd";
const char kChromeCastPrefix[] =
    "*://*.gvt1.com/edgedl/chromewebstore/*pkedcjkdefgpdelpbcmbmeomcjbeemfm*";

const char kWidevineGvt1Prefix[] =
    "*://*.gvt1.com/*oimompecagnajdejgnnjijobebaeigek*";
const char kWidevineGoogleDlPrefix[] =
    "*://dl.google.com/*oimompecagnajdejgnnjijobebaeigek*";

const char kUserAgentHeader[] = "User-Agent";
const char kBravePartnerHeader[] = "X-Brave-Partner";
const char kBraveServicesKeyHeader[] = "BraveServiceKey";

const char kBittorrentMimeType[] = "application/x-bittorrent";
const char kOctetStreamMimeType[] = "application/octet-stream";

const char kSecGpcHeader[] = "Sec-GPC";
