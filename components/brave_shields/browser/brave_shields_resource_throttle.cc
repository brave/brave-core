/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_resource_throttle.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
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
  bool allow_ad_block = brave_shields::IsAllowContentSettingFromIO(
      request_, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS, "ad-block");
  bool allow_tracking_protection = brave_shields::IsAllowContentSettingFromIO(
      request_, tab_origin,
      CONTENT_SETTINGS_TYPE_PLUGINS, "tracking-protection");
  if (allow_ad_block &&
      !g_brave_browser_process->tracking_protection_service()->
      ShouldStartRequest(request_->url(), resource_type_, tab_origin.host())) {
    Cancel();
    brave_shields::DispatchBlockedEventFromIO(request_, "adBlock");
  }
  if (allow_tracking_protection &&
      !g_brave_browser_process->ad_block_service()->
      ShouldStartRequest(request_->url(), resource_type_, tab_origin.host())) {
    Cancel();
    brave_shields::DispatchBlockedEventFromIO(request_, "trackingProtection");
  }
}
