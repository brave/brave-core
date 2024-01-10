// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_controller.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/one_shot_event.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/l10n/common/locale_util.h"
#include "url/origin.h"

namespace brave_news {

namespace {
mojom::Publisher* FindMatchPreferringLocale(
    const Publishers& publishers,
    const std::string& preferred_locale,
    base::RepeatingCallback<bool(const mojom::Publisher&)> matcher) {
  if (publishers.empty()) {
    return nullptr;
  }

  mojom::Publisher* match = nullptr;
  for (const auto& [_, publisher] : publishers) {
    if (!matcher.Run(*publisher)) {
      continue;
    }

    auto locale_it = base::ranges::find(publisher->locales, preferred_locale,
                                        &mojom::LocaleInfo::locale);
    // If the match is in our preferred locale, return it.
    if (locale_it != publisher->locales.end()) {
      return publisher.get();
    }

    // Otherwise, if we don't have a match yet, make this our match (so we
    // prefer whatever we found first).
    if (!match) {
      match = publisher.get();
    }
  }

  // If we didn't have a match in our preferred locale, return the first
  // matching publisher.
  return match;
}
}  // namespace

PublishersController::PublishersController(
    PrefService* prefs,
    DirectFeedController* direct_feed_controller,
    UnsupportedPublisherMigrator* unsupported_publisher_migrator,
    api_request_helper::APIRequestHelper* api_request_helper)
    : prefs_(prefs),
      direct_feed_controller_(direct_feed_controller),
      unsupported_publisher_migrator_(unsupported_publisher_migrator),
      api_request_helper_(api_request_helper),
      on_current_update_complete_(new base::OneShotEvent()) {}

PublishersController::~PublishersController() = default;

const mojom::Publisher* PublishersController::GetPublisherForSite(
    const GURL& site_url) const {
  const auto& site_host = site_url.host();

  // Can't match a Publisher from an empty host
  if (site_host.empty()) {
    return nullptr;
  }

  return FindMatchPreferringLocale(
      publishers_, default_locale_,
      base::BindRepeating(
          [](const std::string& site_host, const mojom::Publisher& publisher) {
            return publisher.site_url.host() == site_host;
          },
          site_host));
}

const mojom::Publisher* PublishersController::GetPublisherForFeed(
    const GURL& feed_url) const {
  return FindMatchPreferringLocale(
      publishers_, default_locale_,
      base::BindRepeating(
          [](const GURL& feed_url, const mojom::Publisher& publisher) {
            return publisher.feed_source == feed_url;
          },
          feed_url));
}

const Publishers& PublishersController::GetLastPublishers() const {
  return publishers_;
}

void PublishersController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PublishersController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// To be consumed outside of the class - provides a clone
void PublishersController::GetOrFetchPublishers(
    GetPublishersCallback callback,
    bool wait_for_current_update /* = false */) {
  if (!GetIsEnabled(prefs_)) {
    std::move(callback).Run({});
    return;
  }

  GetOrFetchPublishers(
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
void PublishersController::GetOrFetchPublishers(base::OnceClosure callback,
                                                bool wait_for_current_update) {
  // If in-memory data is already present, no need to wait,
  // otherwise wait for fetch to be complete.
  // Also don't wait if there's an update in progress and this caller
  // wishes to wait.
  if (!publishers_.empty() &&
      (!wait_for_current_update || !is_update_in_progress_)) {
    std::move(callback).Run();
    return;
  }
  // Ensure data is currently being fetched and subscribe to know
  // when that is complete.
  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
  EnsurePublishersIsUpdating();
}

void PublishersController::GetLocale(
    mojom::BraveNewsController::GetLocaleCallback callback) {
  GetOrFetchPublishers(base::BindOnce(
      [](PublishersController* controller,
         mojom::BraveNewsController::GetLocaleCallback callback, Publishers _) {
        std::move(callback).Run(controller->default_locale_);
      },
      base::Unretained(this), std::move(callback)));
}

const std::string& PublishersController::GetLastLocale() const {
  return default_locale_;
}

void PublishersController::EnsurePublishersIsUpdating() {
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_current_update_complete_` OneShotEvent.
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;

  GURL sources_url(
      base::StrCat({"https://", brave_news::GetHostname(), "/sources.",
                    brave_news::kRegionUrlPart, "json"}));
  auto on_request = base::BindOnce(
      [](PublishersController* controller,
         api_request_helper::APIRequestResult api_request_result) {
        // TODO(petemill): handle bad status or response
        std::optional<Publishers> publisher_list =
            ParseCombinedPublisherList(api_request_result.value_body());

        // Update failed, we'll just reuse whatever publishers we had before.
        if (!publisher_list) {
          controller->on_current_update_complete_->Signal();
          controller->is_update_in_progress_ = false;
          controller->on_current_update_complete_ =
              std::make_unique<base::OneShotEvent>();
          return;
        }

        // Add user enabled statuses
        const auto& publisher_prefs =
            controller->prefs_->GetDict(prefs::kBraveNewsSources);
        std::vector<std::string> missing_publishers;
        for (const auto&& [key, value] : publisher_prefs) {
          auto publisher_id = key;
          auto is_user_enabled = value.GetIfBool();
          if (publisher_list->contains(publisher_id) &&
              is_user_enabled.has_value()) {
            (*publisher_list)[publisher_id]->user_enabled_status =
                (is_user_enabled.value()
                     ? brave_news::mojom::UserEnabled::ENABLED
                     : brave_news::mojom::UserEnabled::DISABLED);
          } else {
            VLOG(1) << "Publisher list did not contain publisher found in"
                       "user prefs: "
                    << publisher_id
                    << ". This could be because we've removed the publisher. "
                       "Attempting to migrate to a direct feed.";
            // We only care about missing publishers if the user was subscribed
            // to them.
            if (is_user_enabled.value_or(false)) {
              missing_publishers.push_back(publisher_id);
            }
          }
        }
        // Add direct feeds
        std::vector<mojom::PublisherPtr> direct_publishers =
            controller->direct_feed_controller_->ParseDirectFeedsPref();
        for (auto it = direct_publishers.begin(); it != direct_publishers.end();
             it++) {
          auto move_it = std::make_move_iterator(it);
          auto publisher = *move_it;
          publisher_list->insert_or_assign(publisher->publisher_id,
                                           std::move(publisher));
        }

        // Set memory cache
        controller->publishers_ = std::move(*publisher_list);
        controller->UpdateDefaultLocale();
        // Let any callback know that the data is ready.
        VLOG(1) << "Notify subscribers to publishers data";
        // One-shot subscribers
        controller->on_current_update_complete_->Signal();
        controller->is_update_in_progress_ = false;
        controller->on_current_update_complete_ =
            std::make_unique<base::OneShotEvent>();
        // Observers
        for (auto& observer : controller->observers_) {
          observer.OnPublishersUpdated(controller);
        }

        if (!missing_publishers.empty()) {
          controller->unsupported_publisher_migrator_->MigrateUnsupportedFeeds(
              missing_publishers,
              base::BindOnce(
                  [](PublishersController* controller,
                     uint64_t migrated_count) {
                    // If any publisher was migrated, ensure we update the list
                    // of publishers.
                    if (migrated_count != 0) {
                      controller->EnsurePublishersIsUpdating();
                    }
                  },
                  base::Unretained(controller)));
        }
      },
      base::Unretained(this));
  api_request_helper_->Request(
      "GET", sources_url, "", "", std::move(on_request),
      brave::private_cdn_headers, {.auto_retry_on_network_change = true});
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
