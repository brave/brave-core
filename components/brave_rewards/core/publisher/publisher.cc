/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/legacy/media/media.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/publisher/prefix_util.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"
#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal {
namespace publisher {

Publisher::Publisher(RewardsEngineImpl& engine)
    : engine_(engine),
      prefix_list_updater_(engine),
      server_publisher_fetcher_(engine) {}

Publisher::~Publisher() = default;

bool Publisher::ShouldFetchServerPublisherInfo(
    mojom::ServerPublisherInfo* server_info) {
  return server_publisher_fetcher_.IsExpired(server_info);
}

void Publisher::FetchServerPublisherInfo(
    const std::string& publisher_key,
    database::GetServerPublisherInfoCallback callback) {
  server_publisher_fetcher_.Fetch(publisher_key, callback);
}

void Publisher::RefreshPublisher(const std::string& publisher_key,
                                 RefreshPublisherCallback callback) {
  // Bypass cache and unconditionally fetch the latest info
  // for the specified publisher.
  server_publisher_fetcher_.Fetch(publisher_key, [callback](auto server_info) {
    auto status = server_info ? server_info->status
                              : mojom::PublisherStatus::NOT_VERIFIED;
    callback(status);
  });
}

void Publisher::SetPublisherServerListTimer() {
  prefix_list_updater_.StartAutoUpdate(
      [this]() { engine_->client()->OnPublisherRegistryUpdated(); });
}

void Publisher::CalcScoreConsts(const int min_duration_seconds) {
  // we increase duration for 100 to keep it as close to muon implementation
  // as possible (we used 1000 in muon)
  // keeping it with only seconds visits are not spaced out equally
  uint64_t min_duration_big = min_duration_seconds * 100;
  const double d = 1.0 / (30.0 * 1000.0);
  const double a = (1.0 / (d * 2.0)) - min_duration_big;
  const double b = min_duration_big - a;

  engine_->state()->SetScoreValues(a, b);
}

// courtesy of @dimitry-xyz:
// https://github.com/brave/engine/issues/2#issuecomment-221752002
double Publisher::concaveScore(const uint64_t& duration_seconds) {
  uint64_t duration_big = duration_seconds * 100;
  double a, b;
  engine_->state()->GetScoreValues(&a, &b);
  return (-b + std::sqrt((b * b) + (a * 4 * duration_big))) / (a * 2);
}

std::string getProviderName(const std::string& publisher_id) {
  // TODO(anyone) this is for the media stuff
  if (publisher_id.find(YOUTUBE_MEDIA_TYPE) != std::string::npos) {
    return YOUTUBE_MEDIA_TYPE;
  } else if (publisher_id.find(GITHUB_MEDIA_TYPE) != std::string::npos) {
    return GITHUB_MEDIA_TYPE;
  }

  return "";
}

bool ignoreMinTime(const std::string& publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void Publisher::SaveVisit(const std::string& publisher_key,
                          const mojom::VisitData& visit_data,
                          const uint64_t duration,
                          const bool first_visit,
                          uint64_t window_id,
                          const PublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher key is empty";
    return;
  }

  auto on_server_info =
      std::bind(&Publisher::OnSaveVisitServerPublisher, this, _1, publisher_key,
                visit_data, duration, first_visit, window_id, callback);

  engine_->database()->SearchPublisherPrefixList(
      publisher_key,
      [this, publisher_key, on_server_info](bool publisher_exists) {
        if (publisher_exists) {
          GetServerPublisherInfo(publisher_key, on_server_info);
        } else {
          on_server_info(nullptr);
        }
      });
}

mojom::ActivityInfoFilterPtr Publisher::CreateActivityFilter(
    const std::string& publisher_id,
    mojom::ExcludeFilter excluded,
    bool min_duration,
    const uint64_t& current_reconcile_stamp,
    bool non_verified,
    bool min_visits) {
  auto filter = mojom::ActivityInfoFilter::New();
  filter->id = publisher_id;
  filter->excluded = excluded;
  filter->min_duration =
      min_duration ? engine_->state()->GetPublisherMinVisitTime() : 0;
  filter->reconcile_stamp = current_reconcile_stamp;
  filter->non_verified = non_verified;
  filter->min_visits =
      min_visits ? engine_->state()->GetPublisherMinVisits() : 0;

  return filter;
}

void Publisher::OnSaveVisitServerPublisher(
    mojom::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const mojom::VisitData& visit_data,
    const uint64_t duration,
    const bool first_visit,
    uint64_t window_id,
    const PublisherInfoCallback callback) {
  auto filter = CreateActivityFilter(
      publisher_key, mojom::ExcludeFilter::FILTER_ALL, false,
      engine_->state()->GetReconcileStamp(), true, false);

  // we need to do this as I can't move server publisher into final function
  auto status = mojom::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  PublisherInfoCallback get_callback =
      std::bind(&Publisher::SaveVisitInternal, this, status, publisher_key,
                visit_data, duration, first_visit, window_id, callback, _1, _2);

  auto list_callback = std::bind(&Publisher::OnGetActivityInfo, this, _1,
                                 get_callback, filter->id);

  engine_->database()->GetActivityInfoList(0, 2, std::move(filter),
                                           list_callback);
}

void Publisher::OnGetActivityInfo(std::vector<mojom::PublisherInfoPtr> list,
                                  PublisherInfoCallback callback,
                                  const std::string& publisher_key) {
  if (list.empty()) {
    engine_->database()->GetPublisherInfo(publisher_key, callback);
    return;
  }

  if (list.size() > 1) {
    callback(mojom::Result::TOO_MANY_RESULTS, nullptr);
    return;
  }

  callback(mojom::Result::OK, std::move(list[0]));
}

void Publisher::SaveVisitInternal(const mojom::PublisherStatus status,
                                  const std::string& publisher_key,
                                  const mojom::VisitData& visit_data,
                                  const uint64_t duration,
                                  const bool first_visit,
                                  uint64_t window_id,
                                  const PublisherInfoCallback callback,
                                  mojom::Result result,
                                  mojom::PublisherInfoPtr publisher_info) {
  DCHECK(result != mojom::Result::TOO_MANY_RESULTS);
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Visit was not saved " << result;
    callback(mojom::Result::FAILED, nullptr);
    return;
  }

  bool is_verified = IsVerified(status);

  bool new_publisher = false;
  bool updated_publisher = false;
  if (!publisher_info) {
    new_publisher = true;
    publisher_info = mojom::PublisherInfo::New();
    publisher_info->id = publisher_key;
  } else if (publisher_info->name != visit_data.name ||
             publisher_info->url != visit_data.url) {
    updated_publisher = true;
  }

  std::string fav_icon = visit_data.favicon_url;
  if (is_verified && !fav_icon.empty()) {
    if (fav_icon.find(".invalid") == std::string::npos) {
      engine_->client()->FetchFavIcon(
          fav_icon,
          "https://" + base::Uuid::GenerateRandomV4().AsLowercaseString() +
              ".invalid",
          base::BindOnce(&Publisher::onFetchFavIcon, base::Unretained(this),
                         publisher_info->id, window_id));
    } else {
      publisher_info->favicon_url = fav_icon;
    }
  } else {
    publisher_info->favicon_url = constant::kClearFavicon;
  }

  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->status = status;

  bool excluded = publisher_info->excluded == mojom::PublisherExclude::EXCLUDED;
  bool ignore_time = ignoreMinTime(publisher_key);
  if (duration == 0) {
    ignore_time = false;
  }

  mojom::PublisherInfoPtr panel_info = nullptr;

  uint64_t min_visit_time =
      static_cast<uint64_t>(engine_->state()->GetPublisherMinVisitTime());

  // for new visits that are excluded or are not long enough or ac is off
  bool min_duration_new = duration < min_visit_time && !ignore_time;
  bool min_duration_ok = duration > min_visit_time || ignore_time;

  if ((new_publisher || updated_publisher) &&
      (excluded || !engine_->state()->GetAutoContributeEnabled() ||
       min_duration_new || !is_verified)) {
    panel_info = publisher_info->Clone();

    auto publisher_info_saved_callback =
        std::bind(&Publisher::OnPublisherInfoSaved, this, _1);

    engine_->database()->SavePublisherInfo(std::move(publisher_info),
                                           publisher_info_saved_callback);
  } else if (!excluded && min_duration_ok && is_verified) {
    if (first_visit) {
      publisher_info->visits += 1;
    }
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = engine_->state()->GetReconcileStamp();

    // Activity queries expect the publisher to exist in the `publisher_info`
    // table. Save the publisher info if it does not already exist.
    if (new_publisher) {
      engine_->database()->SavePublisherInfo(publisher_info->Clone(),
                                             [](mojom::Result) {});
    }

    panel_info = publisher_info->Clone();

    auto publisher_info_saved_callback =
        std::bind(&Publisher::OnPublisherInfoSaved, this, _1);

    engine_->database()->SaveActivityInfo(std::move(publisher_info),
                                          publisher_info_saved_callback);
  }

  if (panel_info) {
    if (panel_info->favicon_url == constant::kClearFavicon) {
      panel_info->favicon_url = std::string();
    }

    auto callback_info = panel_info->Clone();
    callback(mojom::Result::OK, std::move(callback_info));

    if (window_id > 0) {
      OnPanelPublisherInfo(mojom::Result::OK, std::move(panel_info), window_id,
                           visit_data);
    }
  }
}

void Publisher::onFetchFavIcon(const std::string& publisher_key,
                               uint64_t window_id,
                               bool success,
                               const std::string& favicon_url) {
  if (!success || favicon_url.empty()) {
    engine_->Log(FROM_HERE) << "Corrupted favicon file";
    return;
  }

  engine_->database()->GetPublisherInfo(
      publisher_key, std::bind(&Publisher::onFetchFavIconDBResponse, this, _1,
                               _2, favicon_url, window_id));
}

void Publisher::onFetchFavIconDBResponse(mojom::Result result,
                                         mojom::PublisherInfoPtr info,
                                         const std::string& favicon_url,
                                         uint64_t window_id) {
  if (result != mojom::Result::OK || favicon_url.empty()) {
    engine_->Log(FROM_HERE) << "Missing or corrupted favicon file";
    return;
  }

  info->favicon_url = favicon_url;

  auto callback = std::bind(&Publisher::OnPublisherInfoSaved, this, _1);

  engine_->database()->SavePublisherInfo(info->Clone(), callback);

  if (window_id > 0) {
    mojom::VisitData visit_data;
    OnPanelPublisherInfo(mojom::Result::OK, std::move(info), window_id,
                         visit_data);
  }
}

void Publisher::OnPublisherInfoSaved(const mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Publisher info was not saved";
    return;
  }

  SynopsisNormalizer();
}

void Publisher::SetPublisherExclude(const std::string& publisher_id,
                                    const mojom::PublisherExclude& exclude,
                                    ResultCallback callback) {
  auto get_publisher_info_callback =
      base::BindOnce(&Publisher::OnSetPublisherExclude, base::Unretained(this),
                     std::move(callback), exclude);

  engine_->database()->GetPublisherInfo(
      publisher_id,
      [callback = std::make_shared<decltype(get_publisher_info_callback)>(
           std::move(get_publisher_info_callback))](
          mojom::Result result, mojom::PublisherInfoPtr publisher_info) {
        std::move(*callback).Run(result, std::move(publisher_info));
      });
}

void Publisher::OnSetPublisherExclude(ResultCallback callback,
                                      mojom::PublisherExclude exclude,
                                      mojom::Result result,
                                      mojom::PublisherInfoPtr publisher_info) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Publisher exclude status not saved";
    std::move(callback).Run(result);
    return;
  }

  if (!publisher_info) {
    engine_->LogError(FROM_HERE) << "Publisher is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (publisher_info->excluded == exclude) {
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  publisher_info->excluded = exclude;

  auto save_callback = std::bind(&Publisher::OnPublisherInfoSaved, this, _1);
  engine_->database()->SavePublisherInfo(publisher_info->Clone(),
                                         save_callback);
  if (exclude == mojom::PublisherExclude::EXCLUDED) {
    engine_->database()->DeleteActivityInfo(publisher_info->id,
                                            [](const mojom::Result _) {});
  }
  std::move(callback).Run(mojom::Result::OK);
}

void Publisher::OnRestorePublishers(mojom::Result result,
                                    ResultCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Could not restore publishers.";
    std::move(callback).Run(result);
    return;
  }

  SynopsisNormalizer();
  std::move(callback).Run(mojom::Result::OK);
}

void Publisher::NormalizeContributeWinners(
    std::vector<mojom::PublisherInfoPtr>* newList,
    const std::vector<mojom::PublisherInfoPtr>* list,
    uint32_t record) {
  synopsisNormalizerInternal(newList, list, record);
}

void Publisher::synopsisNormalizerInternal(
    std::vector<mojom::PublisherInfoPtr>* newList,
    const std::vector<mojom::PublisherInfoPtr>* list,
    uint32_t /* next_record */) {
  if (list->empty()) {
    engine_->Log(FROM_HERE) << "Publisher list is empty";
    return;
  }

  double totalScores = 0.0;
  for (const auto& entry : *list) {
    totalScores += entry->score;
  }

  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (const auto& entry : *list) {
    double floatNumber = (entry->score / totalScores) * 100.0;
    double roundNumber = (unsigned int)std::lround(floatNumber);
    realPercents.push_back(floatNumber);
    percents.push_back(roundNumber);
    double roundoff = roundNumber - floatNumber;
    if (roundoff < 0.0) {
      roundoff *= -1.0;
    }
    roundoffs.push_back(roundoff);
    totalPercents += roundNumber;
    weights.push_back(floatNumber);
  }
  while (totalPercents != 100) {
    size_t valueToChange = 0;
    double currentRoundOff = 0.0;
    for (size_t i = 0; i < percents.size(); i++) {
      if (i == 0) {
        currentRoundOff = roundoffs[i];
        continue;
      }
      if (roundoffs[i] > currentRoundOff) {
        currentRoundOff = roundoffs[i];
        valueToChange = i;
      }
    }
    if (percents.size() != 0) {
      if (totalPercents > 100) {
        if (percents[valueToChange] != 0) {
          percents[valueToChange] -= 1;
          totalPercents -= 1;
        }
      } else {
        if (percents[valueToChange] != 100) {
          percents[valueToChange] += 1;
          totalPercents += 1;
        }
      }
      roundoffs[valueToChange] = 0;
    }
  }
  size_t currentValue = 0;
  for (const auto& entry : *list) {
    entry->percent = percents[currentValue];
    entry->weight = weights[currentValue];
    currentValue++;
    if (newList) {
      newList->push_back(entry->Clone());
    }
  }
}

void Publisher::SynopsisNormalizer() {
  auto filter =
      CreateActivityFilter("", mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
                           true, engine_->state()->GetReconcileStamp(), false,
                           engine_->state()->GetPublisherMinVisits());
  engine_->database()->GetActivityInfoList(
      0, 0, std::move(filter),
      std::bind(&Publisher::SynopsisNormalizerCallback, this, _1));
}

void Publisher::SynopsisNormalizerCallback(
    std::vector<mojom::PublisherInfoPtr> list) {
  std::vector<mojom::PublisherInfoPtr> normalized_list;
  synopsisNormalizerInternal(&normalized_list, &list, 0);
  std::vector<mojom::PublisherInfoPtr> save_list;
  for (auto& item : list) {
    save_list.push_back(item.Clone());
  }

  engine_->database()->NormalizeActivityInfoList(std::move(save_list),
                                                 [](const mojom::Result) {});
}

bool Publisher::IsVerified(mojom::PublisherStatus status) {
  return status != mojom::PublisherStatus::NOT_VERIFIED;
}

void Publisher::GetPublisherActivityFromUrl(uint64_t windowId,
                                            mojom::VisitDataPtr visit_data,
                                            const std::string& publisher_blob) {
  if (!visit_data) {
    return;
  }

  const bool is_media = visit_data->domain == YOUTUBE_DOMAIN ||
                        visit_data->domain == GITHUB_DOMAIN;

  if (is_media && visit_data->path != "" && visit_data->path != "/") {
    std::string type = YOUTUBE_MEDIA_TYPE;
    if (visit_data->domain == GITHUB_DOMAIN) {
      type = GITHUB_MEDIA_TYPE;
    }

    if (!visit_data->url.empty()) {
      visit_data->url.pop_back();
    }

    visit_data->url += visit_data->path;

    engine_->media()->GetMediaActivityFromUrl(windowId, std::move(visit_data),
                                              type, publisher_blob);
    return;
  }

  auto filter = CreateActivityFilter(
      visit_data->domain, mojom::ExcludeFilter::FILTER_ALL, false,
      engine_->state()->GetReconcileStamp(), true, false);

  visit_data->favicon_url = "";

  engine_->database()->GetPanelPublisherInfo(
      std::move(filter), std::bind(&Publisher::OnPanelPublisherInfo, this, _1,
                                   _2, windowId, *visit_data));
}

void Publisher::OnSaveVisitInternal(mojom::Result result,
                                    mojom::PublisherInfoPtr info) {
  // TODO(nejczdovc): handle if needed
}

void Publisher::OnPanelPublisherInfo(mojom::Result result,
                                     mojom::PublisherInfoPtr info,
                                     uint64_t windowId,
                                     const mojom::VisitData& visit_data) {
  if (result == mojom::Result::OK) {
    engine_->client()->OnPanelPublisherInfo(result, std::move(info), windowId);
    return;
  }

  if (result == mojom::Result::NOT_FOUND && !visit_data.domain.empty()) {
    auto callback = std::bind(&Publisher::OnSaveVisitInternal, this, _1, _2);

    SaveVisit(visit_data.domain, visit_data, 0, true, windowId, callback);
  }
}

void Publisher::GetPublisherBanner(const std::string& publisher_key,
                                   GetPublisherBannerCallback callback) {
  const auto banner_callback = std::bind(&Publisher::OnGetPublisherBanner, this,
                                         _1, publisher_key, callback);

  // NOTE: We do not attempt to search the prefix list before getting
  // the publisher data because if the prefix list was not properly
  // loaded then the user would not see the correct banner information
  // for a verified publisher. Assuming that the user has explicitly
  // requested this information by interacting with the UI, we should
  // make a best effort to return correct and updated information even
  // if the prefix list is incorrect.
  GetServerPublisherInfo(publisher_key, banner_callback);
}

void Publisher::OnGetPublisherBanner(mojom::ServerPublisherInfoPtr info,
                                     const std::string& publisher_key,
                                     GetPublisherBannerCallback callback) {
  auto banner = mojom::PublisherBanner::New();

  if (info) {
    if (info->banner) {
      banner = info->banner->Clone();
    }

    banner->status = info->status;
  }

  banner->publisher_key = publisher_key;

  const auto publisher_callback =
      std::bind(&Publisher::OnGetPublisherBannerPublisher, this, callback,
                *banner, _1, _2);

  engine_->database()->GetPublisherInfo(publisher_key, publisher_callback);
}

void Publisher::OnGetPublisherBannerPublisher(
    GetPublisherBannerCallback callback,
    const mojom::PublisherBanner& banner,
    mojom::Result result,
    mojom::PublisherInfoPtr publisher_info) {
  auto new_banner = mojom::PublisherBanner::New(banner);

  if (!publisher_info || result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Publisher info not found";
    callback(std::move(new_banner));
    return;
  }

  new_banner->name = publisher_info->name;
  new_banner->provider = publisher_info->provider;

  if (new_banner->logo.empty()) {
    new_banner->logo = publisher_info->favicon_url;
  }

  callback(std::move(new_banner));
}

void Publisher::GetServerPublisherInfo(
    const std::string& publisher_key,
    database::GetServerPublisherInfoCallback callback) {
  GetServerPublisherInfo(publisher_key, false, std::move(callback));
}

void Publisher::GetServerPublisherInfo(
    const std::string& publisher_key,
    bool use_prefix_list,
    database::GetServerPublisherInfoCallback callback) {
  engine_->database()->GetServerPublisherInfo(
      publisher_key, std::bind(&Publisher::OnServerPublisherInfoLoaded, this,
                               _1, publisher_key, use_prefix_list, callback));
}

void Publisher::OnServerPublisherInfoLoaded(
    mojom::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    bool use_prefix_list,
    database::GetServerPublisherInfoCallback callback) {
  if (!server_info && use_prefix_list) {
    // If we don't have a record in the database for this publisher, search the
    // prefix list. If the prefix list indicates that the publisher is likely
    // registered, then fetch the publisher data.
    engine_->database()->SearchPublisherPrefixList(
        publisher_key, [this, publisher_key, callback](bool publisher_exists) {
          if (publisher_exists) {
            FetchServerPublisherInfo(
                publisher_key, [callback](mojom::ServerPublisherInfoPtr info) {
                  callback(std::move(info));
                });
          } else {
            callback(nullptr);
          }
        });
    return;
  }

  if (ShouldFetchServerPublisherInfo(server_info.get())) {
    // Store the current server publisher info so that if fetching fails
    // we can execute the callback with the last known valid data.
    auto shared_info =
        std::make_shared<mojom::ServerPublisherInfoPtr>(std::move(server_info));

    FetchServerPublisherInfo(
        publisher_key,
        [shared_info, callback](mojom::ServerPublisherInfoPtr info) {
          callback(std::move(info ? info : *shared_info));
        });
    return;
  }

  callback(std::move(server_info));
}

void Publisher::UpdateMediaDuration(const uint64_t window_id,
                                    const std::string& publisher_key,
                                    const uint64_t duration,
                                    const bool first_visit) {
  engine_->Log(FROM_HERE) << "Media duration: " << duration;
  engine_->database()->GetPublisherInfo(
      publisher_key,
      std::bind(&Publisher::OnGetPublisherInfoForUpdateMediaDuration, this, _1,
                _2, window_id, duration, first_visit));
}

void Publisher::OnGetPublisherInfoForUpdateMediaDuration(
    mojom::Result result,
    mojom::PublisherInfoPtr info,
    const uint64_t window_id,
    const uint64_t duration,
    const bool first_visit) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Failed to retrieve publisher info while updating media duration";
    return;
  }

  mojom::VisitData visit_data;
  visit_data.name = info->name;
  visit_data.url = info->url;
  visit_data.provider = info->provider;
  visit_data.favicon_url = info->favicon_url;

  SaveVisit(info->id, visit_data, duration, first_visit, 0,
            [](mojom::Result, mojom::PublisherInfoPtr) {});
}

void Publisher::GetPublisherPanelInfo(const std::string& publisher_key,
                                      GetPublisherPanelInfoCallback callback) {
  auto filter = CreateActivityFilter(
      publisher_key, mojom::ExcludeFilter::FILTER_ALL, false,
      engine_->state()->GetReconcileStamp(), true, false);

  engine_->database()->GetPanelPublisherInfo(
      std::move(filter),
      std::bind(&Publisher::OnGetPanelPublisherInfo, this, _1, _2, callback));
}

void Publisher::OnGetPanelPublisherInfo(
    const mojom::Result result,
    mojom::PublisherInfoPtr info,
    GetPublisherPanelInfoCallback callback) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to retrieve panel publisher info";
    callback(result, nullptr);
    return;
  }

  callback(result, std::move(info));
}

void Publisher::SavePublisherInfo(uint64_t window_id,
                                  mojom::PublisherInfoPtr publisher_info,
                                  LegacyResultCallback callback) {
  if (!publisher_info || publisher_info->id.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher key is missing for url";
    callback(mojom::Result::FAILED);
    return;
  }

  mojom::VisitData visit_data;
  visit_data.provider = publisher_info->provider;
  visit_data.name = publisher_info->name;
  visit_data.url = publisher_info->url;
  if (!publisher_info->favicon_url.empty()) {
    visit_data.favicon_url = publisher_info->favicon_url;
  }

  auto banner_callback =
      std::bind(&Publisher::OnGetPublisherBannerForSavePublisherInfo, this, _1,
                window_id, publisher_info->id, visit_data, callback);

  GetPublisherBanner(publisher_info->id, banner_callback);
}

void Publisher::OnGetPublisherBannerForSavePublisherInfo(
    mojom::PublisherBannerPtr banner,
    uint64_t window_id,
    const std::string& publisher_key,
    const mojom::VisitData& visit_data,
    LegacyResultCallback callback) {
  mojom::VisitData new_visit_data = visit_data;

  if (banner && !banner->logo.empty()) {
    auto index = banner->logo.find("https://");
    if (index != std::string::npos) {
      new_visit_data.favicon_url = std::string(banner->logo, index);
    }
  }

  SaveVisit(publisher_key, new_visit_data, 0, true, window_id,
            [callback](auto result, mojom::PublisherInfoPtr publisher_info) {
              callback(result);
            });
}

// static
std::string Publisher::GetShareURL(
    const base::flat_map<std::string, std::string>& args) {
  auto comment = args.find("comment");
  auto name = args.find("name");
  auto tweet_id = args.find("tweet_id");
  auto hashtag = args.find("hashtag");
  if (comment == args.end() || name == args.end() || hashtag == args.end()) {
    return "";
  }

  // Append hashtag to comment ("%20%23" = percent-escaped space and
  // number sign)
  const std::string comment_with_hashtag =
      comment->second + "%20%23" + hashtag->second;

  // If a tweet ID was specified, then quote the original tweet along
  // with the supplied comment; otherwise, just tweet the comment.
  std::string share_url;
  if (tweet_id != args.end() && !tweet_id->second.empty()) {
    const std::string quoted_tweet_url =
        base::StringPrintf("https://twitter.com/%s/status/%s",
                           name->second.c_str(), tweet_id->second.c_str());
    share_url = base::StringPrintf(
        "https://twitter.com/intent/tweet?text=%s&url=%s",
        comment_with_hashtag.c_str(), quoted_tweet_url.c_str());
  } else {
    share_url = base::StringPrintf("https://twitter.com/intent/tweet?text=%s",
                                   comment_with_hashtag.c_str());
  }

  return share_url;
}

}  // namespace publisher
}  // namespace brave_rewards::internal
