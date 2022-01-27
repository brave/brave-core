// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/feed_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace history {
class HistoryService;
}  // namespace history

namespace brave_news {

// Browser-side handler for Brave News mojom API, 1 per profile
// Orchestrates FeedController and PublishersController for data, as well as
// owning prefs data.
// Controls remote feed update logic via Timer and prefs values.
class BraveNewsController : public KeyedService,
                            public mojom::BraveNewsController {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  BraveNewsController(
      PrefService* prefs,
      brave_ads::AdsService* ads_service,
      history::HistoryService* history_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveNewsController() override;
  BraveNewsController(const BraveNewsController&) = delete;
  BraveNewsController& operator=(const BraveNewsController&) = delete;

  mojo::PendingRemote<mojom::BraveNewsController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveNewsController> receiver);

  // Remove any cache that would identify user browsing history
  void ClearHistory();

  // mojom::BraveNewsController
  void GetFeed(GetFeedCallback callback) override;
  void GetPublishers(GetPublishersCallback callback) override;
  void GetImageData(const GURL& padded_image_url,
                    GetImageDataCallback callback) override;
  void SetPublisherPref(const std::string& publisher_id,
                        mojom::UserEnabled new_status) override;
  void ClearPrefs() override;
  void IsFeedUpdateAvailable(const std::string& displayed_feed_hash,
                             IsFeedUpdateAvailableCallback callback) override;
  void GetDisplayAd(GetDisplayAdCallback callback) override;
  void OnInteractionSessionStarted() override;
  void OnSessionCardVisitsCountChanged(
      uint16_t cards_visited_session_total_count) override;
  void OnSessionCardViewsCountChanged(
      uint16_t cards_viewed_session_total_count) override;
  void OnPromotedItemView(const std::string& item_id,
                          const std::string& creative_instance_id) override;
  void OnPromotedItemVisit(const std::string& item_id,
                           const std::string& creative_instance_id) override;
  void OnDisplayAdVisit(const std::string& item_id,
                        const std::string& creative_instance_id) override;
  void OnDisplayAdView(const std::string& item_id,
                       const std::string& creative_instance_id) override;

 private:
  void ConditionallyStartOrStopTimer();
  void CheckForFeedsUpdate();
  void CheckForPublishersUpdate();
  void Prefetch();

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  api_request_helper::APIRequestHelper api_request_helper_;
  PublishersController publishers_controller_;
  FeedController feed_controller_;

  PrefChangeRegistrar pref_change_registrar_;
  base::OneShotTimer timer_prefetch_;
  base::RepeatingTimer timer_feed_update_;
  base::RepeatingTimer timer_publishers_update_;

  mojo::ReceiverSet<mojom::BraveNewsController> receivers_;
  base::WeakPtrFactory<BraveNewsController> weak_ptr_factory_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_CONTROLLER_H_
