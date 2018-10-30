/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_REWARDS_FETCHER_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_REWARDS_FETCHER_SERVICE_OBSERVER_H_

#include <string>

#include "base/callback.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "url/gurl.h"

namespace brave_rewards {
  using OnImageChangedCallback = base::Callback<void(
        const std::string& favicon_key_,
        const GURL& url,
        const BitmapFetcherService::RequestId& request_id,
        const SkBitmap &answers_image)>;

class RewardsFetcherServiceObserver : public BitmapFetcherService::Observer {
  public:
    RewardsFetcherServiceObserver(const std::string& favicon_key,
                                  const GURL& url,
                                  const OnImageChangedCallback& callback);
    ~RewardsFetcherServiceObserver() override;
    void OnImageChanged(BitmapFetcherService::RequestId request_id,
                        const SkBitmap& answers_image) override;

  protected:
    std::string favicon_key_;
    GURL url_;
    OnImageChangedCallback callback_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_REWARDS_FETCHER_SERVICE_OBSERVER_H_
