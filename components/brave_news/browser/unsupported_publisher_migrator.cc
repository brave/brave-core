// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/l10n/common/locale_util.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "url/gurl.h"

namespace brave_news {

UnsupportedPublisherMigrator::UnsupportedPublisherMigrator(
    PrefService* prefs,
    DirectFeedController* direct_feed_controller,
    api_request_helper::APIRequestHelper* api_request_helper)
    : prefs_(prefs),
      direct_feed_controller_(direct_feed_controller),
      api_request_helper_(api_request_helper),
      on_init_complete_(std::make_unique<base::OneShotEvent>()) {}

UnsupportedPublisherMigrator::~UnsupportedPublisherMigrator() = default;

void UnsupportedPublisherMigrator::EnsureInitialized() {
  if (initialized_) {
    return;
  }

  std::string locale =
      brave_l10n::GetDefaultISOLanguageCodeString() == "ja" ? "ja" : "";
  GURL sources_url("https://" + brave_news::GetHostname() + "/sources." +
                   locale + "json");
  auto on_response = base::BindOnce(
      [](UnsupportedPublisherMigrator* migrator,
         api_request_helper::APIRequestResult result) {
        VLOG(1) << "Downloaded old sources, status: " << result.response_code();

        // Only parse the publishers if the response was successful. If not we
        // can try and migrate the sources again next time the browser is
        // launched.
        if (result.Is2XXResponseCode()) {
          std::optional<Publishers> publishers =
              ParseCombinedPublisherList(result.value_body());
          if (publishers) {
            migrator->v1_api_publishers_ = std::move(*publishers);
          }
        }

        migrator->on_init_complete_->Signal();
      },
      base::Unretained(this));

  api_request_helper_->Request(
      "GET", sources_url, "", "", std::move(on_response),
      brave::private_cdn_headers, {.auto_retry_on_network_change = true});
  initialized_ = true;
}

void UnsupportedPublisherMigrator::MigrateUnsupportedFeeds(
    const std::vector<std::string>& unsupported_ids,
    MigratedCallback callback) {
  EnsureInitialized();
  if (!on_init_complete_->is_signaled()) {
    on_init_complete_->Post(
        FROM_HERE,
        base::BindOnce(&UnsupportedPublisherMigrator::MigrateUnsupportedFeeds,
                       base::Unretained(this), unsupported_ids,
                       std::move(callback)));
    return;
  }

  uint64_t migrated_count = 0;

  for (const auto& publisher_id : unsupported_ids) {
    auto it = v1_api_publishers_.find(publisher_id);

    if (it == v1_api_publishers_.end()) {
      VLOG(1) << "Encountered unknown publisher id: " << publisher_id
              << " which wasn't removed in the migration to the v2 API";
      continue;
    }
    // As we found a match, add it as a direct feed. This may fail if the feed
    // already exists, but that's fine (because it will still show up).
    direct_feed_controller_->AddDirectFeedPref(
        it->second->feed_source, it->second->publisher_name, publisher_id);

    // Once we've added the direct feed, delete the feed from our combined
    // publishers list.
    ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsSources);
    update->Remove(publisher_id);
    migrated_count++;
  }
  std::move(callback).Run(migrated_count);
}

}  // namespace brave_news
