// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

namespace p3a {
class NewsMetrics;
}  // namespace p3a

class DirectFeedController;
using GetPublishersCallback = mojom::BraveNewsController::GetPublishersCallback;
using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;

class PublishersController {
 public:
  PublishersController(
      PrefService* prefs,
      DirectFeedController* direct_feed_controller,
      UnsupportedPublisherMigrator* unsupported_publisher_migrator,
      api_request_helper::APIRequestHelper* api_request_helper,
      p3a::NewsMetrics* news_metrics);
  ~PublishersController();
  PublishersController(const PublishersController&) = delete;
  PublishersController& operator=(const PublishersController&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPublishersUpdated(PublishersController* controller) = 0;
  };

  // The following methods return a pointer to a |Publisher|. This pointer is
  // not safe to hold onto, as the object it points to will be destroyed when
  // the publishers are updated (which happens regularly). If you need it to
  // live longer, take a clone.
  const mojom::Publisher* GetPublisherForSite(const GURL& site_url) const;
  const mojom::Publisher* GetPublisherForFeed(const GURL& feed_url) const;
  const Publishers& GetLastPublishers() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void GetOrFetchPublishers(GetPublishersCallback callback,
                            bool wait_for_current_update = false);
  void GetLocale(mojom::BraveNewsController::GetLocaleCallback);
  const std::string& GetLastLocale() const;
  void EnsurePublishersIsUpdating();
  void ClearCache();

 private:
  void GetOrFetchPublishers(base::OnceClosure callback,
                            bool wait_for_current_update);
  void UpdateDefaultLocale();

  raw_ptr<PrefService> prefs_;
  raw_ptr<DirectFeedController> direct_feed_controller_;
  raw_ptr<UnsupportedPublisherMigrator> unsupported_publisher_migrator_;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  raw_ptr<p3a::NewsMetrics> news_metrics_;

  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;
  base::ObserverList<Observer> observers_;
  std::string default_locale_;
  Publishers publishers_;
  bool is_update_in_progress_ = false;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_CONTROLLER_H_
