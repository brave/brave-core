// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/suggestions_controller.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/barrier_callback.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/one_shot_event.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/channels_controller.h"
#include "brave/components/brave_today/browser/locales_helper.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

namespace brave_news {
namespace {
constexpr double kVisitedMin = 0.4;
constexpr double kVisitedMax = 1;

constexpr double kSimilarSubscribedMin = 0;
constexpr double kSimilarSubscribedMax = 0.4;

constexpr double kSimilarVisitedMin = 0;
constexpr double kSimilarVisitedMax = 0.3;

// Projects a normalized value in the [0, 1] range to a new range.
double ProjectToRange(double value, double min, double max) {
  const double range = max - min;
  return value * range + min;
}

base::flat_map<std::string, double> GetVisitWeightings(
    const Publishers& publishers,
    const history::QueryResults& history) {
  base::flat_set<std::string> publisher_hosts;
  for (const auto& [_, publisher] : publishers) {
    publisher_hosts.insert(publisher->site_url.host());
  }

  base::flat_map<std::string, double> weightings;
  for (const auto& entry : history)
    weightings[entry.url().host()] += 1;

  if (!weightings.size())
    return weightings;

  auto max_visits =
      base::ranges::max(weightings, [](const auto& a, const auto& b) {
        return a.second < b.second;
      }).second;

  // Normalize the visit counts by dividing by the maximum number of visits.
  for (auto& it : weightings) {
    it.second /= max_visits;
  }
  return weightings;
}

// Get score for having visited a source.
double GetVisitWeighting(const mojom::PublisherPtr& publisher,
                         base::flat_map<std::string, double> visit_weightings) {
  const auto host_name = publisher->site_url.host();
  auto it = visit_weightings.find(host_name);
  if (it == visit_weightings.end()) {
    // The |site_urls| we receive from Brave News aren't terribly accurate, and
    // many of them are missing bits and pieces. This is a simple middle ground
    // while we wait for them to be fixed.
    // Relevant issue: https://github.com/brave/news-aggregator/issues/58
    if (!base::StartsWith(host_name, "www.")) {
      it = visit_weightings.find("www." + host_name);
    }

    if (it == visit_weightings.end()) {
      return 0;
    }
  }

  return ProjectToRange(it->second, kVisitedMin, kVisitedMax);
}

SuggestionsController::SimilarityLookup ParseSimilarityResponse(
    const std::string& json,
    const std::string& locale) {
  SuggestionsController::SimilarityLookup result;
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return result;
  }

  if (!records_v->is_dict()) {
    return result;
  }

  SuggestionsController::PublisherSimilarities similarities;

  for (const auto it : records_v->GetDict()) {
    const auto& for_publisher = it.first;
    const auto& similarity_list = it.second.GetList();
    for (const auto& similarity : similarity_list) {
      const auto& dict = similarity.GetDict();
      similarities[for_publisher].push_back(
          {.publisher_id = *dict.FindString("source"),
           .score = dict.FindDouble("score").value()});
    }
  }

  result[locale] = std::move(similarities);
  return result;
}

}  // namespace
SuggestionsController::SuggestionsController(
    PrefService* prefs,
    ChannelsController* channels_controller,
    PublishersController* publishers_controller,
    api_request_helper::APIRequestHelper* api_request_helper,
    history::HistoryService* history_service)
    : prefs_(prefs),
      channels_controller_(channels_controller),
      publishers_controller_(publishers_controller),
      api_request_helper_(api_request_helper),
      history_service_(history_service),
      on_current_update_complete_(new base::OneShotEvent()) {}
SuggestionsController::~SuggestionsController() = default;

void SuggestionsController::GetSuggestedPublisherIds(
    const std::string& locale,
    GetSuggestedPublisherIdsCallback callback) {
  GetOrFetchSimilarityMatrix(base::BindOnce(
      [](SuggestionsController* controller, std::string locale,
         GetSuggestedPublisherIdsCallback callback) {
        controller->publishers_controller_->GetOrFetchPublishers(base::BindOnce(
            [](SuggestionsController* controller, std::string locale,
               GetSuggestedPublisherIdsCallback callback,
               Publishers publishers) {
              history::QueryOptions options;
              options.max_count = 2000;
              options.SetRecentDayRange(14);
              controller->history_service_->QueryHistory(
                  std::u16string(), options,
                  base::BindOnce(&SuggestionsController::
                                     GetSuggestedPublisherIdsWithHistory,
                                 base::Unretained(controller), locale,
                                 std::move(publishers), std::move(callback)),
                  &controller->task_tracker_);
            },
            base::Unretained(controller), locale, std::move(callback)));
      },
      base::Unretained(this), locale, std::move(callback)));
}

void SuggestionsController::GetSuggestedPublisherIdsWithHistory(
    const std::string& locale,
    Publishers publishers,
    GetSuggestedPublisherIdsCallback callback,
    history::QueryResults history) {
  if (!similarity_lookup_.contains(locale)) {
    std::move(callback).Run({});
    return;
  }

  const auto visit_weightings = GetVisitWeightings(publishers, history);
  PublisherSimilarities* similarities;
  auto similarity_it = similarity_lookup_.find(locale);
  if (similarity_it != similarity_lookup_.end()) {
    similarities = &similarity_it->second;
  }

  base::flat_map<std::string, double> scores;

  for (const auto& [publisher_id, publisher] : publishers) {
    // If this publisher isn't available in the current locale we don't want
    // it to affect our suggestions.
    if (!base::Contains(publisher->locales, locale)) {
      continue;
    }

    const bool explicitly_enabled =
        publisher->user_enabled_status == mojom::UserEnabled::ENABLED;
    const auto visited_score = GetVisitWeighting(publisher, visit_weightings);
    const bool visited = visited_score != 0;

    if (!explicitly_enabled) {
      scores[publisher_id] += visited_score;
    }

    // Only consider similar sources if we have visited this one or it has been
    // explicitly enabled.
    if (!visited && !explicitly_enabled) {
      continue;
    }

    // If we have no similarities, nothing more to do here.
    if (!similarities) {
      continue;
    }

    // If there are no similar publishers, we have nothing more to do here.
    const auto& similarity_info_it = similarities->find(publisher_id);
    if (similarity_info_it == similarities->end()) {
      continue;
    }

    for (const auto& info : similarity_info_it->second) {
      const auto& similar_publisher = publishers.at(info.publisher_id);
      // Don't suggest similar publishers which are already enabled.
      // TODO(fallaciousreasoning): Maybe we shouldn't suggest explicitly
      // disabled publishers either?
      if (similar_publisher->user_enabled_status ==
          mojom::UserEnabled::ENABLED) {
        continue;
      }

      // TODO(fallaciousreasoning): Should |visit_score| be multiplied by the
      // visit weight for the original source?
      auto visit_score = visited
                             ? ProjectToRange(info.score, kSimilarVisitedMin,
                                              kSimilarVisitedMax)
                             : 0;
      auto subscribed_score =
          explicitly_enabled ? ProjectToRange(info.score, kSimilarSubscribedMin,
                                              kSimilarSubscribedMax)
                             : 0;
      scores[info.publisher_id] += visit_score + subscribed_score;
    }
  }

  std::vector<std::string> result;
  for (const auto& [publisher_id, score] : scores) {
    // Either the source it was similar to was disabled, or
    // the source is already enabled.
    if (score == 0)
      continue;
    result.push_back(publisher_id);
  }

  std::sort(result.begin(), result.end(),
            [scores](const std::string& a_id, const std::string& b_id) {
              return scores.at(a_id) > scores.at(b_id);
            });

  std::move(callback).Run(std::move(result));
}

void SuggestionsController::EnsureSimilarityMatrixIsUpdating() {
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;

  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      [](SuggestionsController* controller, Publishers publishers) {
        auto locales = GetMinimalLocalesSet(
            controller->channels_controller_->GetChannelLocales(), publishers);
        auto completed_callback = base::BarrierCallback<SimilarityLookup>(
            locales.size(),
            base::BindOnce(
                [](SuggestionsController* controller,
                   std::vector<SimilarityLookup> similarity_matrices) {
                  SimilarityLookup flattened;
                  for (const auto& locale_lookup : similarity_matrices) {
                    for (const auto& [key, value] : locale_lookup) {
                      flattened[std::move(key)] = std::move(value);
                    }
                  }

                  controller->similarity_lookup_ = std::move(flattened);
                  controller->on_current_update_complete_->Signal();
                  controller->is_update_in_progress_ = false;
                  controller->on_current_update_complete_ =
                      std::make_unique<base::OneShotEvent>();
                },
                base::Unretained(controller)));

        for (const auto& locale : locales) {
          const GURL url("https://" + brave_today::GetHostname() +
                         "/source-suggestions/source_similarity_t10." + locale +
                         ".json");
          controller->api_request_helper_->Request(
              "GET", url, "", "", true,
              base::BindOnce(
                  [](SuggestionsController* controller, std::string locale,
                     base::RepeatingCallback<void(SimilarityLookup)> callback,
                     api_request_helper::APIRequestResult api_request_result) {
                    callback.Run(ParseSimilarityResponse(
                        api_request_result.body(), locale));
                  },
                  base::Unretained(controller), locale, completed_callback));
        }
      },
      base::Unretained(this)));
}

void SuggestionsController::GetOrFetchSimilarityMatrix(
    base::OnceClosure callback) {
  if (!similarity_lookup_.empty() && !is_update_in_progress_) {
    std::move(callback).Run();
    return;
  }

  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
  EnsureSimilarityMatrixIsUpdating();
}
}  // namespace brave_news
