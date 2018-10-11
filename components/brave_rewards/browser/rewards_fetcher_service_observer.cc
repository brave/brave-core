/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_fetcher_service_observer.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "url/gurl.h"

namespace brave_rewards {

RewardsFetcherServiceObserver::RewardsFetcherServiceObserver(
    const std::string& favicon_key,
    const GURL& url,
    const OnImageChangedCallback& callback) :
  favicon_key_(favicon_key),
  url_(url),
  callback_(callback) {
}

RewardsFetcherServiceObserver::~RewardsFetcherServiceObserver() {}

void RewardsFetcherServiceObserver::OnImageChanged(BitmapFetcherService::RequestId request_id,
                                                   const SkBitmap& answers_image) {
  if (callback_) {
    callback_.Run(favicon_key_, url_, request_id, answers_image);
  }
}

}  // namespace brave_rewards
