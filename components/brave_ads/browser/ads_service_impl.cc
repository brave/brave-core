/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include "bat/ads/ads.h"
#include "bat/ads/url_session.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_ads {

AdsServiceImpl::AdsServiceImpl(Profile* profile) : profile_(profile) {
  DCHECK(!profile_->IsOffTheRecord());

  if (is_enabled()) {
    ads_.reset(ads::Ads::CreateInstance(this));
  }
}

AdsServiceImpl::~AdsServiceImpl() {}

bool AdsServiceImpl::is_enabled() {
  return true;
}

std::string AdsServiceImpl::SetLocale(const std::string& locale) {
  return "";
}

std::unique_ptr<ads::URLSession> AdsServiceImpl::URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLSession::Method& method,
      ads::URLSessionCallbackHandlerCallback callback) {
  return nullptr;
}

}  // namespace brave_ads
