// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_UNSUPPORTED_PUBLISHER_MIGRATOR_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_UNSUPPORTED_PUBLISHER_MIGRATOR_H_

#include <memory>
#include <string>
#include <vector>
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/one_shot_event.h"
#include "base/timer/timer.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

using MigratedCallback = base::OnceCallback<void(uint64_t migrated)>;

// This class is used to migrate publishers which we used to manage on the
// server but no longer do.
//
// This class will make one API call, the first time it is used to get a list
// of all historically available publishers. When encountering an unknown
// publisher other services can ask us to migrate them to a direct feed.
//
// At that point we will do the following:
// 1. Get the list of historic publishers, if we don't have them already.
// 2. Look up the publisher id.
// 3. If it exists in the historic publishers, add it's feed url to our direct
//    publishers list.
// 4. Remove the publisher from our combined sources.
//
// Publishers which aren't in our list of historic sources are not modified.
class UnsupportedPublisherMigrator {
 public:
  UnsupportedPublisherMigrator(
      PrefService* prefs,
      DirectFeedController* direct_feed_controller,
      api_request_helper::APIRequestHelper* api_request_helper);
  ~UnsupportedPublisherMigrator();
  UnsupportedPublisherMigrator(const UnsupportedPublisherMigrator&) = delete;
  UnsupportedPublisherMigrator& operator=(const UnsupportedPublisherMigrator&) =
      delete;

  void MigrateUnsupportedFeeds(const std::vector<std::string>& unsupported_ids,
                               MigratedCallback callback);

 private:
  void EnsureInitialized();

  bool initialized_ = false;

  raw_ptr<PrefService> prefs_;
  raw_ptr<DirectFeedController> direct_feed_controller_;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::flat_map<std::string, mojom::PublisherPtr> v1_api_publishers_;
  std::unique_ptr<base::OneShotEvent> on_init_complete_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_UNSUPPORTED_PUBLISHER_MIGRATOR_H_
