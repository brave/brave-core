/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <string>
#include <vector>

#include "base/base64url.h"
#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"


namespace brave {

bool IsEmptyDataURLRedirect(const GURL& gurl) {
  static std::vector<std::string> hosts({
    "sp1.nypost.com",
    "sp.nasdaq.com"
  });
  return std::find(hosts.begin(), hosts.end(), gurl.host()) !=
      hosts.end();
}

bool IsBlockedResource(const GURL& gurl) {
  static std::vector<URLPattern> blocked_patterns({
    URLPattern(URLPattern::SCHEME_ALL, "https://www.lesechos.fr/xtcore.js"),
    URLPattern(URLPattern::SCHEME_ALL, "https://*.y8.com/js/sdkloader/outstream.js")
  });
  return std::any_of(blocked_patterns.begin(), blocked_patterns.end(),
      [&gurl](URLPattern pattern){
        return pattern.MatchesURL(gurl);
      });
}

std::string GetGoogleTagManagerPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str;
  str.reserve(135);
  str.append("(function() { var noopfn = function() { ; }; window.ga = window.ga || noopfn; })();");
  base64_output.reserve(180);
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING, &base64_output);
  base64_output= std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

std::string GetGoogleTagServicesPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str;
  str.reserve(3500);
  str.append("(function() { var p; var noopfn = function() { }; var noopthisfn = function() { return this; }; var noopnullfn = function() { return null; }; var nooparrayfn = function() { return []; }; var noopstrfn = function() { return ''; }; var companionAdsService = { addEventListener: noopthisfn, enableSyncLoading: noopfn, setRefreshUnfilledSlots: noopfn }; var contentService = { addEventListener: noopthisfn, setContent: noopfn }; var PassbackSlot = function() { }; p = PassbackSlot.prototype; p.display =");
  str.append(" noopfn; p.get = noopnullfn; p.set = noopthisfn; p.setClickUrl = noopthisfn; p.setTagForChildDirectedTreatment = noopthisfn; p.setTargeting = noopthisfn; p.updateTargetingFromMap = noopthisfn; var pubAdsService = { addEventListener: noopthisfn, clear: noopfn, clearCategoryExclusions: noopthisfn, clearTagForChildDirectedTreatment: noopthisfn, clearTargeting: noopthisfn, collapseEmptyDivs: noopfn, defineOutOfPagePassback: function() { return new PassbackSlot(); }, definePassback: function() { ret");
  str.append("urn new PassbackSlot(); }, disableInitialLoad: noopfn, display: noopfn, enableAsyncRendering: noopfn, enableSingleRequest: noopfn, enableSyncRendering: noopfn, enableVideoAds: noopfn, get: noopnullfn, getAttributeKeys: nooparrayfn, refresh: noopfn, set: noopthisfn, setCategoryExclusion: noopthisfn, setCentering: noopfn, setCookieOptions: noopthisfn, setLocation: noopthisfn, setPublisherProvidedId: noopthisfn, setTagForChildDirectedTreatment: noopthisfn, setTargeting: noopthisfn, setVideoContent");
  str.append(": noopthisfn, updateCorrelator: noopfn }; var SizeMappingBuilder = function() { }; p = SizeMappingBuilder.prototype; p.addSize = noopthisfn; p.build = noopnullfn; var Slot = function() { }; p = Slot.prototype; p.addService = noopthisfn; p.clearCategoryExclusions = noopthisfn; p.clearTargeting = noopthisfn; p.defineSizeMapping = noopthisfn; p.get = noopnullfn; p.getAdUnitPath = nooparrayfn; p.getAttributeKeys = nooparrayfn; p.getCategoryExclusions = nooparrayfn; p.getDomId = noopstrfn; p.getSlot");
  str.append("ElementId = noopstrfn; p.getSlotId = noopthisfn; p.getTargeting = nooparrayfn; p.getTargetingKeys = nooparrayfn; p.set = noopthisfn; p.setCategoryExclusion = noopthisfn; p.setClickUrl = noopthisfn; p.setCollapseEmptyDiv = noopthisfn; p.setTargeting = noopthisfn; var gpt = window.googletag || {}; window.googletag.destroySlots = function () { }; var cmd = gpt.cmd || []; gpt.apiReady = true; gpt.cmd = []; gpt.cmd.push = function(a) { try { a(); } catch (ex) { } return 1; }; gpt.companionAds = func");
  str.append("tion() { return companionAdsService; }; gpt.content = function() { return contentService; }; gpt.defineOutOfPageSlot = function() { return new Slot(); }; gpt.defineSlot = function() { return new Slot(); }; gpt.disablePublisherConsole = noopfn; gpt.display = noopfn; gpt.enableServices = noopfn; gpt.getVersion = noopstrfn; gpt.pubads = function() { return pubAdsService; }; gpt.pubadsReady = true; gpt.sizeMapping = function() { return new SizeMappingBuilder(); }; window.googletag = gpt; while ( cm");
  str.append("d.length !== 0 ) { gpt.cmd.push(cmd.shift()); } })();");
  base64_output.reserve(4668);
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING, &base64_output);
  base64_output= std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

bool GetPolyfill(const GURL& gurl, GURL *new_url) {
  static URLPattern tag_manager(URLPattern::SCHEME_ALL, kGoogleTagManagerPattern);
  static URLPattern tag_services(URLPattern::SCHEME_ALL, kGoogleTagServicesPattern);
  if (tag_manager.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagManagerPolyfillJS();
    *new_url = GURL(data_url);
  } else if (tag_services.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagServicesPolyfillJS();
    *new_url = GURL(data_url);
  }
  return net::OK;
}

int OnBeforeURLRequest_SiteHacksWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<OnBeforeURLRequestContext> ctx) {
  const GURL& url = request->url();

  if (IsEmptyDataURLRedirect(url)) {
    *new_url = GURL(kEmptyDataURI);
    return net::OK;
  }

  if (IsBlockedResource(url)) {
    request->Cancel();
    return net::ERR_ABORTED;
  }

  if (GetPolyfill(url, new_url)) {
    return net::OK;
  }

  return net::OK;
}

}  // namespace brave
