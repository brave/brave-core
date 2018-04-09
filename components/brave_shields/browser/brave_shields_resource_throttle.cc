/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_resource_throttle.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "net/url_request/url_request.h"


content::ResourceThrottle* MaybeCreateBraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type) {
  return new BraveShieldsResourceThrottle(request,
      resource_type);
}

BraveShieldsResourceThrottle::BraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type) :
      request_(request),
      resource_type_(resource_type) {
}

BraveShieldsResourceThrottle::~BraveShieldsResourceThrottle() = default;

const char* BraveShieldsResourceThrottle::GetNameForLogging() const {
  return "BraveShieldsResourceThrottle";
}

void BraveShieldsResourceThrottle::WillStartRequest(bool* defer) {
  GURL tab_origin = request_->site_for_cookies().GetOrigin();
  // Proper content settings can't be looked up, so do nothing.
  if (tab_origin.is_empty()) {
    return;
  }
  bool allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request_, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  bool allow_ads = brave_shields::IsAllowContentSettingFromIO(
      request_, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);
  bool allow_trackers = brave_shields::IsAllowContentSettingFromIO(
      request_, tab_origin, tab_origin,
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kTrackers);
  if (allow_brave_shields &&
      !allow_trackers &&
      !g_brave_browser_process->tracking_protection_service()->
      ShouldStartRequest(request_->url(), resource_type_, tab_origin.host())) {
    Cancel();
    brave_shields::DispatchBlockedEventFromIO(request_,
        brave_shields::kTrackers);
  }
  if (allow_brave_shields &&
      !allow_ads &&
      !g_brave_browser_process->ad_block_service()->
      ShouldStartRequest(request_->url(), resource_type_, tab_origin.host())) {
    Cancel();
    brave_shields::DispatchBlockedEventFromIO(request_,
        brave_shields::kAds);
  }
}
