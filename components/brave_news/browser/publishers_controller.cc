// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_controller.h"

#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/one_shot_event.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/l10n/common/locale_util.h"
#include "url/origin.h"

namespace brave_news {

namespace {
mojom::PublisherPtr FindMatchPreferringLocale(
    const Publishers& publishers,
    const std::string& preferred_locale,
    base::RepeatingCallback<bool(const mojom::Publisher&)> matcher) {
  if (publishers.empty()) {
    return nullptr;
  }

  mojom::PublisherPtr match = nullptr;
  for (const auto& [_, publisher] : publishers) {
    if (!matcher.Run(*publisher)) {
      continue;
    }

    auto locale_it = base::ranges::find(publisher->locales, preferred_locale,
                                        &mojom::LocaleInfo::locale);
    // If the match is in our preferred locale, return it.
    if (locale_it != publisher->locales.end()) {
      return publisher->Clone();
    }

    // Otherwise, if we don't have a match yet, make this our match (so we
    // prefer whatever we found first).
    if (!match) {
      match = publisher->Clone();
    }
  }

  // If we didn't have a match in our preferred locale, return the first
  // matching publisher.
  return match;
}

// Apart from fetching, we need to make sure the subscriptions are up to date.
void ApplySubscriptions(Publishers& publishers,
                        const SubscriptionsSnapshot& subscriptions) {
  DVLOG(1) << __FUNCTION__;
  // Remove all direct feeds - they'll get re-added.
  for (auto it = publishers.begin(); it != publishers.end();) {
    if (it->second->type == mojom::PublisherType::DIRECT_SOURCE) {
      publishers.erase(it);
    } else {
      it++;
    }
  }

  // Update the user subscription status.
  for (auto& [id, publisher] : publishers) {
    if (subscriptions.enabled_publishers().contains(id)) {
      publisher->user_enabled_status = mojom::UserEnabled::ENABLED;
    } else if (subscriptions.disabled_publishers().contains(id)) {
      publisher->user_enabled_status = mojom::UserEnabled::DISABLED;
    } else {
      publisher->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;
    }
  }

  // Add direct feeds
  std::vector<mojom::PublisherPtr> direct_publishers;
  ParseDirectPublisherList(subscriptions.direct_feeds(), &direct_publishers);
  for (auto it = direct_publishers.begin(); it != direct_publishers.end();
       it++) {
    auto move_it = std::make_move_iterator(it);
    auto publisher = *move_it;
    publishers.insert_or_assign(publisher->publisher_id, std::move(publisher));
  }
}

}  // namespace

bool IsSubscribed(const mojom::PublisherPtr& publisher) {
  return publisher->user_enabled_status == mojom::UserEnabled::ENABLED ||
         publisher->type == mojom::PublisherType::DIRECT_SOURCE;
}

PublishersController::PublishersController(
    api_request_helper::APIRequestHelper* api_request_helper)
    : api_request_helper_(api_request_helper) {}

PublishersController::~PublishersController() = default;

void PublishersController::GetPublisherForSite(
    const SubscriptionsSnapshot& subscriptions,
    const GURL& site_url,
    GetPublisherCallback callback) {
  GetOrFetchPublishers(
      subscriptions,
      base::BindOnce(
          [](PublishersController* controller, GURL site_url,
             GetPublisherCallback callback, Publishers publishers) {
            const auto& site_host = site_url.host();

            // Can't match a Publisher from an empty host
            if (site_host.empty()) {
              std::move(callback).Run(nullptr);
              return;
            }

            std::move(callback).Run(FindMatchPreferringLocale(
                publishers, controller->default_locale_,
                base::BindRepeating(
                    [](const std::string& site_host,
                       const mojom::Publisher& publisher) {
                      return publisher.site_url.host() == site_host;
                    },
                    site_host)));
          },
          base::Unretained(this), site_url, std::move(callback)),
      false);
}

void PublishersController::GetPublisherForFeed(
    const SubscriptionsSnapshot& subscriptions,
    const GURL& feed_url,
    GetPublisherCallback callback) {
  std::move(callback).Run(FindMatchPreferringLocale(
      publishers_, default_locale_,
      base::BindRepeating(
          [](const GURL& feed_url, const mojom::Publisher& publisher) {
            return publisher.feed_source == feed_url;
          },
          feed_url)));
}

// To be consumed outside of the class - provides a clone
void PublishersController::GetOrFetchPublishers(
    const SubscriptionsSnapshot& subscriptions,
    GetPublishersCallback callback,
    bool wait_for_current_update /* = false */) {
  GetOrFetchPublishers(
      subscriptions,
      base::BindOnce(
          [](PublishersController* controller, GetPublishersCallback callback) {
            // Either there was already data, or the fetch was complete
            // (with success or error, so we would still check for valid data
            // again, but it's fine to just send the empty array). Provide data
            // clone for ownership outside of this class.
            Publishers clone;
            for (auto const& kv : controller->publishers_) {
              clone.insert_or_assign(kv.first, kv.second->Clone());
            }
            std::move(callback).Run(std::move(clone));
          },
          base::Unretained(this), std::move(callback)),
      wait_for_current_update);
}

// To be consumed internally - provides no data so that we don't need to clone,
// as data can be accessed via class property
void PublishersController::GetOrFetchPublishers(
    const SubscriptionsSnapshot& subscriptions,
    base::OnceClosure callback,
    bool wait_for_current_update) {
  // If in-memory data is already present, no need to wait,
  // otherwise wait for fetch to be complete.
  // Also don't wait if there's an update in progress and this caller
  // wishes to wait.
  if (!publishers_.empty() &&
      (!wait_for_current_update || !on_current_update_complete_)) {
    // Make sure the subscriptions are up to date.
    DVLOG(1) << "Not refetching publishers, responding from cache.";
    ApplySubscriptions(publishers_, subscriptions);
    std::move(callback).Run();
    return;
  }

  // Ensure data is currently being fetched and subscribe to know
  // when that is complete.
  EnsurePublishersIsUpdating(subscriptions);
  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
}

void PublishersController::GetLocale(
    const SubscriptionsSnapshot& subscriptions,
    mojom::BraveNewsController::GetLocaleCallback callback) {
  GetOrFetchPublishers(
      subscriptions,
      base::BindOnce(
          [](PublishersController* controller,
             mojom::BraveNewsController::GetLocaleCallback callback,
             Publishers _) {
            VLOG(1) << "Got locale: " << controller->default_locale_;
            std::move(callback).Run(controller->default_locale_);
          },
          base::Unretained(this), std::move(callback)));
}

const std::string& PublishersController::GetLastLocale() const {
  return default_locale_;
}

void PublishersController::EnsurePublishersIsUpdating(
    const SubscriptionsSnapshot& subscriptions) {
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_current_update_complete_` OneShotEvent.
  if (on_current_update_complete_) {
    return;
  }
  on_current_update_complete_ = std::make_unique<base::OneShotEvent>();

  GURL sources_url(
      base::StrCat({"https://", brave_news::GetHostname(), "/sources.",
                    brave_news::kRegionUrlPart, "json"}));
  VLOG(1) << "Fetching publishers from " << sources_url.spec();

  auto on_request = base::BindOnce(
      [](PublishersController* controller,
         const SubscriptionsSnapshot& subscriptions,
         api_request_helper::APIRequestResult api_request_result) {
        VLOG(1) << "Publishers response status code: "
                << api_request_result.response_code()
                << ", error code: " << api_request_result.error_code()
                << ", final_url: " << api_request_result.final_url();
        // TODO(petemill): handle bad status or response
        std::optional<Publishers> publisher_list =
            ParseCombinedPublisherList(api_request_result.TakeBody());

        // Update failed, we'll just reuse whatever publishers we had before.
        if (!publisher_list) {
          DVLOG(1) << "Failed to fetch publisher list";
          controller->on_current_update_complete_->Signal();
          controller->on_current_update_complete_.reset();
          return;
        }

        ApplySubscriptions(*publisher_list, subscriptions);

        // Set memory cache
        controller->publishers_ = std::move(*publisher_list);
        controller->UpdateDefaultLocale();
        // Let any callback know that the data is ready.
        VLOG(1) << "Notify subscribers to publishers data";
        // One-shot subscribers
        controller->on_current_update_complete_->Signal();
        controller->on_current_update_complete_.reset();
      },
      base::Unretained(this), subscriptions);
  api_request_helper_->Request("GET", sources_url, "", "",
                               std::move(on_request),
                               brave::private_cdn_headers,
                               {.auto_retry_on_network_change = true,
                                .timeout = GetDefaultRequestTimeout()});
}

void PublishersController::UpdateDefaultLocale() {
  auto available_locales = GetPublisherLocales(publishers_);

  // Locale can be "language_Script_COUNTRY.charset@variant" but Brave News
  // wants the format to be "language_COUNTRY".
  const std::string brave_news_locale =
      base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                    brave_l10n::GetDefaultISOCountryCodeString()});

  // Fallback to en_US, if we can't match anything else.
  // TODO(fallaciousreasoning): Implement more complicated fallback
  default_locale_ = base::Contains(available_locales, brave_news_locale)
                        ? brave_news_locale
                        : "en_US";
}

void PublishersController::ClearCache() {
  publishers_.clear();
}

}  // namespace brave_news
