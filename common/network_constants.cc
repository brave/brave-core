/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/network_constants.h"

const char kBraveUpdatesExtensionsEndpoint[] =
    "https://go-updater.brave.com/extensions";
// For debgugging:
// const char kBraveUpdatesExtensionsEndpoint[] =
// "http://localhost:8192/extensions";

const char kBraveReferralsServer[] = "laptop-updates.brave.com";
const char kBraveReferralsHeadersPath[] = "/promo/custom-headers";
const char kBraveReferralsInitPath[] = "/promo/initialize/nonua";
const char kBraveReferralsActivityPath[] = "/promo/activity";

const char kBraveSafeBrowsingFileCheckProxy[] = "sb-ssl.brave.com";
const char kBraveRedirectorProxy[] = "redirector.brave.com";
const char kBraveClients4Proxy[] = "clients4.brave.com";

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
const char kSafeBrowsingFileCheckPrefix[] = "https://sb-ssl.google.com/";
const char kCRLSetPrefix1[] =
    "*://dl.google.com/release2/chrome_component/*crl-set*";
const char kCRLSetPrefix2[] =
    "*://*.gvt1.com/edgedl/release2/chrome_component/*";
const char kCRLSetPrefix3[] =
    "*://www.google.com/dl/release2/chrome_component/*crl-set*";
const char kCRLSetPrefix4[] =
    "*://storage.googleapis.com/update-delta/hfnkpimlhhgieaddgfemjhofmfblmnib"
    "/*crxd";
const char kGoogleAnalyticsPattern[] =
    "https://www.google-analytics.com/analytics.js";
const char kChromeCastPrefix[] =
    "*://*.gvt1.com/edgedl/chromewebstore/*pkedcjkdefgpdelpbcmbmeomcjbeemfm*";
const char kGoogleTagManagerPattern[] =
    "https://www.googletagmanager.com/gtm.js";
const char kGoogleTagServicesPattern[] =
    "https://www.googletagservices.com/tag/js/gpt.js";
const char kForbesPattern[] = "https://www.forbes.com/*";
const char kForbesExtraCookies[] =
    "forbes_ab=true; welcomeAd=true; adblock_session=Off; "
    "dailyWelcomeCookie=true";
const char kTwitterPattern[] = "https://*.twitter.com/*";
const char kTwitterReferrer[] = "https://twitter.com/*";
const char kTwitterRedirectURL[] = "https://mobile.twitter.com/i/nojs_router*";

const char kCookieHeader[] = "Cookie";
// Intentional misspelling on referrer to match HTTP spec
const char kRefererHeader[] = "Referer";
const char kUserAgentHeader[] = "User-Agent";
const char kBravePartnerHeader[] = "X-Brave-Partner";

const char kBittorrentMimeType[] = "application/x-bittorrent";
const char kOctetStreamMimeType[] = "application/octet-stream";
