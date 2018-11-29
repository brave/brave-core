#include "brave/common/network_constants.h"

const char kBraveUpdatesExtensionsEndpoint[] = "https://go-updater.brave.com/extensions";
// For debgugging:
// const char kBraveUpdatesExtensionsEndpoint[] = "http://localhost:8192/extensions";

const char kBraveReferralsServer[] = "laptop-updates.brave.com";
const char kBraveReferralsHeadersPath[] = "/promo/custom-headers";
const char kBraveReferralsInitPath[] = "/promo/initialize/nonua";
const char kBraveReferralsActivityPath[] = "/promo/activity";

const char kEmptyDataURI[] = "data:text/plain,";
const char kEmptyImageDataURI[] = "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
const char kJSDataURLPrefix[] = "data:application/javascript;base64,";
const char kGeoLocationsPattern[] = "https://www.googleapis.com/geolocation/v1/geolocate?key=*";
const char kSafeBrowsingPrefix[] = "https://safebrowsing.googleapis.com/";
const char kGoogleTagManagerPattern[] = "https://www.googletagmanager.com/gtm.js";
const char kGoogleTagServicesPattern[] = "https://www.googletagservices.com/tag/js/gpt.js";
const char kForbesPattern[] = "https://www.forbes.com/*";
const char kForbesExtraCookies[] = "forbes_ab=true; welcomeAd=true; adblock_session=Off; dailyWelcomeCookie=true";
const char kTwitterPattern[] = "https://*.twitter.com/*";
const char kTwitterReferrer[] = "https://twitter.com/*";
const char kTwitterRedirectURL[] = "https://mobile.twitter.com/i/nojs_router*";

const char kCookieHeader[] = "Cookie";
// Intentional misspelling on referrer to match HTTP spec
const char kRefererHeader[] = "Referer";
const char kUserAgentHeader[] = "User-Agent";

const char kBittorrentMimeType[] = "application/x-bittorrent";
const char kOctetStreamMimeType[] = "application/octet-stream";
