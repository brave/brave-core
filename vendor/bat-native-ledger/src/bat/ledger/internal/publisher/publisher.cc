/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "bat/ledger/internal/publisher/prefix_util.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/publisher/publisher_prefix_list_updater.h"
#include "bat/ledger/internal/publisher/server_publisher_fetcher.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace publisher {

Publisher::Publisher(LedgerImpl* ledger):
    ledger_(ledger),
    prefix_list_updater_(
        std::make_unique<PublisherPrefixListUpdater>(ledger)),
    server_publisher_fetcher_(
        std::make_unique<ServerPublisherFetcher>(ledger)) {
}

Publisher::~Publisher() = default;

bool Publisher::ShouldFetchServerPublisherInfo(
    type::ServerPublisherInfo* server_info) {
  return server_publisher_fetcher_->IsExpired(server_info);
}

void Publisher::FetchServerPublisherInfo(
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  server_publisher_fetcher_->Fetch(publisher_key, callback);
}

void Publisher::RefreshPublisher(
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  // Bypass cache and unconditionally fetch the latest info
  // for the specified publisher.
  server_publisher_fetcher_->Fetch(publisher_key,
      [this, callback](auto server_info) {
        auto status = server_info
            ? server_info->status
            : type::PublisherStatus::NOT_VERIFIED;

        // If, after refresh, the publisher is now verified
        // attempt to process any pending contributions for
        // unverified publishers.
        if (status == type::PublisherStatus::UPHOLD_VERIFIED ||
            status == type::PublisherStatus::BITFLYER_VERIFIED) {
          ledger_->contribution()->ContributeUnverifiedPublishers();
        }

        callback(status);
      });
}

void Publisher::SetPublisherServerListTimer() {
  prefix_list_updater_->StartAutoUpdate([this]() {
    // Attempt to reprocess any contributions for previously
    // unverified publishers that are now verified.
    ledger_->contribution()->ContributeUnverifiedPublishers();
  });
}

void Publisher::CalcScoreConsts(const int min_duration_seconds) {
  // we increase duration for 100 to keep it as close to muon implementation
  // as possible (we used 1000 in muon)
  // keeping it with only seconds visits are not spaced out equally
  uint64_t min_duration_big = min_duration_seconds * 100;
  const double d = 1.0 / (30.0 * 1000.0);
  const double a = (1.0 / (d * 2.0)) - min_duration_big;
  const double b = min_duration_big - a;

  ledger_->state()->SetScoreValues(a, b);
}

// courtesy of @dimitry-xyz:
// https://github.com/brave/ledger/issues/2#issuecomment-221752002
double Publisher::concaveScore(const uint64_t& duration_seconds) {
  uint64_t duration_big = duration_seconds * 100;
  double a, b;
  ledger_->state()->GetScoreValues(&a, &b);
  return (-b + std::sqrt((b * b) + (a * 4 * duration_big))) / (a * 2);
}

std::string getProviderName(const std::string& publisher_id) {
  // TODO(anyone) this is for the media stuff
  if (publisher_id.find(YOUTUBE_MEDIA_TYPE) != std::string::npos) {
    return YOUTUBE_MEDIA_TYPE;
  } else if (publisher_id.find(TWITCH_MEDIA_TYPE) != std::string::npos) {
    return TWITCH_MEDIA_TYPE;
  } else if (publisher_id.find(TWITTER_MEDIA_TYPE) != std::string::npos) {
    return TWITTER_MEDIA_TYPE;
  } else if (publisher_id.find(VIMEO_MEDIA_TYPE) != std::string::npos) {
    return VIMEO_MEDIA_TYPE;
  } else if (publisher_id.find(GITHUB_MEDIA_TYPE) != std::string::npos) {
    return GITHUB_MEDIA_TYPE;
  }

  return "";
}

bool ignoreMinTime(const std::string& publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void Publisher::SaveVisit(
    const std::string& publisher_key,
    const type::VisitData& visit_data,
    const uint64_t duration,
    const bool first_visit,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    BLOG(0, "Publisher key is empty");
    return;
  }

  auto on_server_info =
      std::bind(&Publisher::OnSaveVisitServerPublisher,
          this,
          _1,
          publisher_key,
          visit_data,
          duration,
          first_visit,
          window_id,
          callback);

  ledger_->database()->SearchPublisherPrefixList(
      publisher_key,
      [this, publisher_key, on_server_info](bool publisher_exists) {
        if (publisher_exists) {
          GetServerPublisherInfo(publisher_key, on_server_info);
        } else {
          on_server_info(nullptr);
        }
      });
}

void Publisher::SaveVideoVisit(
    const std::string& publisher_id,
    const type::VisitData& visit_data,
    uint64_t duration,
    const bool first_visit,
    uint64_t window_id,
    ledger::PublisherInfoCallback callback) {
  if (!ledger_->state()->GetPublisherAllowVideos()) {
    duration = 0;
  }

  SaveVisit(
      publisher_id,
      visit_data,
      duration,
      first_visit,
      window_id,
      callback);
}

type::ActivityInfoFilterPtr Publisher::CreateActivityFilter(
    const std::string& publisher_id,
    type::ExcludeFilter excluded,
    bool min_duration,
    const uint64_t& current_reconcile_stamp,
    bool non_verified,
    bool min_visits) {
  auto filter = type::ActivityInfoFilter::New();
  filter->id = publisher_id;
  filter->excluded = excluded;
  filter->min_duration = min_duration
      ? ledger_->state()->GetPublisherMinVisitTime()
      : 0;
  filter->reconcile_stamp = current_reconcile_stamp;
  filter->non_verified = non_verified;
  filter->min_visits = min_visits
      ? ledger_->state()->GetPublisherMinVisits()
      : 0;

  return filter;
}

void Publisher::OnSaveVisitServerPublisher(
    type::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const type::VisitData& visit_data,
    const uint64_t duration,
    const bool first_visit,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback) {
  auto filter = CreateActivityFilter(
      publisher_key,
      type::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->state()->GetReconcileStamp(),
      true,
      false);

  // we need to do this as I can't move server publisher into final function
  auto status = type::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  ledger::PublisherInfoCallback get_callback =
      std::bind(&Publisher::SaveVisitInternal,
          this,
          status,
          publisher_key,
          visit_data,
          duration,
          first_visit,
          window_id,
          callback,
          _1,
          _2);

  auto list_callback = std::bind(&Publisher::OnGetActivityInfo,
      this,
      _1,
      get_callback,
      filter->id);

  ledger_->database()->GetActivityInfoList(
      0,
      2,
      std::move(filter),
      list_callback);
}

void Publisher::OnGetActivityInfo(
    type::PublisherInfoList list,
    ledger::PublisherInfoCallback callback,
    const std::string& publisher_key) {
  if (list.empty()) {
    ledger_->database()->GetPublisherInfo(publisher_key, callback);
    return;
  }

  if (list.size() > 1) {
    callback(type::Result::TOO_MANY_RESULTS, nullptr);
    return;
  }

  callback(type::Result::LEDGER_OK, std::move(list[0]));
}

void Publisher::SaveVisitInternal(
    const type::PublisherStatus status,
    const std::string& publisher_key,
    const type::VisitData& visit_data,
    const uint64_t duration,
    const bool first_visit,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback,
    type::Result result,
    type::PublisherInfoPtr publisher_info) {
  DCHECK(result != type::Result::TOO_MANY_RESULTS);
  if (result != type::Result::LEDGER_OK &&
      result != type::Result::NOT_FOUND) {
    BLOG(0, "Visit was not saved " << result);
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bool is_verified = IsConnectedOrVerified(status);

  bool new_publisher = false;
  if (!publisher_info) {
    new_publisher = true;
    publisher_info = type::PublisherInfo::New();
    publisher_info->id = publisher_key;
  }

  std::string fav_icon = visit_data.favicon_url;
  if (is_verified && !fav_icon.empty()) {
    if (fav_icon.find(".invalid") == std::string::npos) {
    ledger_->ledger_client()->FetchFavIcon(
        fav_icon,
        "https://" + base::GenerateGUID() + ".invalid",
        std::bind(&Publisher::onFetchFavIcon,
            this,
            publisher_info->id,
            window_id,
            _1,
            _2));
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

  bool excluded =
      publisher_info->excluded == type::PublisherExclude::EXCLUDED;
  bool ignore_time = ignoreMinTime(publisher_key);
  if (duration == 0) {
    ignore_time = false;
  }

  type::PublisherInfoPtr panel_info = nullptr;

  uint64_t min_visit_time = static_cast<uint64_t>(
      ledger_->state()->GetPublisherMinVisitTime());

  // for new visits that are excluded or are not long enough or ac is off
  bool allow_non_verified = ledger_->state()->GetPublisherAllowNonVerified();
  bool min_duration_new = duration < min_visit_time && !ignore_time;
  bool min_duration_ok = duration > min_visit_time || ignore_time;
  bool verified_new = !allow_non_verified && !is_verified;
  bool verified_old = allow_non_verified || is_verified;

  if (new_publisher &&
      (excluded ||
       !ledger_->state()->GetAutoContributeEnabled() ||
       min_duration_new ||
       verified_new)) {
    panel_info = publisher_info->Clone();

    auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
        this,
        _1);

    ledger_->database()->SavePublisherInfo(std::move(publisher_info), callback);
  } else if (!excluded &&
             ledger_->state()->GetAutoContributeEnabled() &&
             min_duration_ok &&
             verified_old) {
    if (first_visit) {
      publisher_info->visits += 1;
    }
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = ledger_->state()->GetReconcileStamp();

    panel_info = publisher_info->Clone();

    auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
        this,
        _1);

    ledger_->database()->SaveActivityInfo(std::move(publisher_info), callback);
  }

  if (panel_info) {
    if (panel_info->favicon_url == constant::kClearFavicon) {
      panel_info->favicon_url = std::string();
    }

    auto callback_info = panel_info->Clone();
    callback(type::Result::LEDGER_OK, std::move(callback_info));

    if (window_id > 0) {
      OnPanelPublisherInfo(type::Result::LEDGER_OK,
                           std::move(panel_info),
                           window_id,
                           visit_data);
    }
  }
}

void Publisher::onFetchFavIcon(const std::string& publisher_key,
                                   uint64_t window_id,
                                   bool success,
                                   const std::string& favicon_url) {
  if (!success || favicon_url.empty()) {
    BLOG(1, "Corrupted favicon file");
    return;
  }

  ledger_->database()->GetPublisherInfo(publisher_key,
      std::bind(&Publisher::onFetchFavIconDBResponse,
                this,
                _1,
                _2,
                favicon_url,
                window_id));
}

void Publisher::onFetchFavIconDBResponse(
    type::Result result,
    type::PublisherInfoPtr info,
    const std::string& favicon_url,
    uint64_t window_id) {
  if (result != type::Result::LEDGER_OK || favicon_url.empty()) {
    BLOG(1, "Missing or corrupted favicon file");
    return;
  }

  info->favicon_url = favicon_url;

  auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
      this,
      _1);

  ledger_->database()->SavePublisherInfo(info->Clone(), callback);

  if (window_id > 0) {
    type::VisitData visit_data;
    OnPanelPublisherInfo(type::Result::LEDGER_OK,
                        std::move(info),
                        window_id,
                        visit_data);
  }
}

void Publisher::OnPublisherInfoSaved(const type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Publisher info was not saved!");
    return;
  }

  SynopsisNormalizer();
}

void Publisher::SetPublisherExclude(
    const std::string& publisher_id,
    const type::PublisherExclude& exclude,
    ledger::ResultCallback callback) {
  ledger_->database()->GetPublisherInfo(
      publisher_id,
      std::bind(&Publisher::OnSetPublisherExclude,
                this,
                exclude,
                _1,
                _2,
                callback));
}

void Publisher::OnSetPublisherExclude(
    type::PublisherExclude exclude,
    type::Result result,
    type::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK &&
      result != type::Result::NOT_FOUND) {
    BLOG(0, "Publisher exclude status not saved");
    callback(result);
    return;
  }

  if (!publisher_info) {
    BLOG(0, "Publisher is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  if (publisher_info->excluded == exclude) {
    callback(type::Result::LEDGER_OK);
    return;
  }

  publisher_info->excluded = exclude;

  auto save_callback = std::bind(&Publisher::OnPublisherInfoSaved,
      this,
      _1);
  ledger_->database()->SavePublisherInfo(
      publisher_info->Clone(),
      save_callback);
  if (exclude == type::PublisherExclude::EXCLUDED) {
    ledger_->database()->DeleteActivityInfo(
      publisher_info->id,
      [](const type::Result _){});
  }
  callback(type::Result::LEDGER_OK);
}

void Publisher::OnRestorePublishers(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Could not restore publishers.");
    callback(result);
    return;
  }

  SynopsisNormalizer();
  callback(type::Result::LEDGER_OK);
}

void Publisher::NormalizeContributeWinners(
    type::PublisherInfoList* newList,
    const type::PublisherInfoList* list,
    uint32_t record) {
  synopsisNormalizerInternal(newList, list, record);
}

void Publisher::synopsisNormalizerInternal(
    type::PublisherInfoList* newList,
    const type::PublisherInfoList* list,
    uint32_t /* next_record */) {
  if (list->empty()) {
    BLOG(1, "Publisher list is empty");
    return;
  }

  double totalScores = 0.0;
  for (size_t i = 0; i < list->size(); i++) {
    totalScores += (*list)[i]->score;
  }

  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (size_t i = 0; i < list->size(); i++) {
    double floatNumber = ((*list)[i]->score / totalScores) * 100.0;
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
  for (size_t i = 0; i < list->size(); i++) {
    (*list)[i]->percent = percents[currentValue];
    (*list)[i]->weight = weights[currentValue];
    currentValue++;
    if (newList) {
      newList->push_back((*list)[i]->Clone());
    }
  }
}

void Publisher::SynopsisNormalizer() {
  auto filter = CreateActivityFilter("",
      type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->state()->GetReconcileStamp(),
      ledger_->state()->GetPublisherAllowNonVerified(),
      ledger_->state()->GetPublisherMinVisits());
  ledger_->database()->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Publisher::SynopsisNormalizerCallback, this, _1));
}

void Publisher::SynopsisNormalizerCallback(
    type::PublisherInfoList list) {
  type::PublisherInfoList normalized_list;
  synopsisNormalizerInternal(&normalized_list, &list, 0);
  type::PublisherInfoList save_list;
  for (auto& item : list) {
    save_list.push_back(item.Clone());
  }

  ledger_->database()->NormalizeActivityInfoList(
      std::move(save_list),
      [](const type::Result){});
}

bool Publisher::IsConnectedOrVerified(const type::PublisherStatus status) {
  return status == type::PublisherStatus::CONNECTED ||
         status == type::PublisherStatus::UPHOLD_VERIFIED ||
         status == type::PublisherStatus::BITFLYER_VERIFIED;
}

void Publisher::GetPublisherActivityFromUrl(
    uint64_t windowId,
    type::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  if (!visit_data) {
    return;
  }

  const bool is_media =
      visit_data->domain == YOUTUBE_TLD ||
      visit_data->domain == TWITCH_TLD ||
      visit_data->domain == TWITTER_TLD ||
      visit_data->domain == REDDIT_TLD ||
      visit_data->domain == VIMEO_TLD ||
      visit_data->domain == GITHUB_TLD;

  if (is_media && visit_data->path != "" && visit_data->path != "/") {
    std::string type = YOUTUBE_MEDIA_TYPE;
    if (visit_data->domain == TWITCH_TLD) {
      type = TWITCH_MEDIA_TYPE;
    } else if (visit_data->domain == TWITTER_TLD) {
      type = TWITTER_MEDIA_TYPE;
    } else if (visit_data->domain == REDDIT_TLD) {
      type = REDDIT_MEDIA_TYPE;
    } else if (visit_data->domain == VIMEO_TLD) {
      type = VIMEO_MEDIA_TYPE;
    } else if (visit_data->domain == GITHUB_TLD) {
      type = GITHUB_MEDIA_TYPE;
    }

    if (!visit_data->url.empty()) {
      visit_data->url.pop_back();
    }

    visit_data->url += visit_data->path;

    ledger_->media()->GetMediaActivityFromUrl(
        windowId,
        std::move(visit_data),
        type,
        publisher_blob);
    return;
  }

  auto filter = CreateActivityFilter(
      visit_data->domain,
      type::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->state()->GetReconcileStamp(),
      true,
      false);

  visit_data->favicon_url = "";

  ledger_->database()->GetPanelPublisherInfo(
      std::move(filter),
      std::bind(&Publisher::OnPanelPublisherInfo,
          this,
          _1,
          _2,
          windowId,
          *visit_data));
}

void Publisher::OnSaveVisitInternal(
    type::Result result,
    type::PublisherInfoPtr info) {
  // TODO(nejczdovc): handle if needed
}

void Publisher::OnPanelPublisherInfo(
    type::Result result,
    type::PublisherInfoPtr info,
    uint64_t windowId,
    const type::VisitData& visit_data) {
  if (result == type::Result::LEDGER_OK) {
    ledger_->ledger_client()->OnPanelPublisherInfo(
        result,
        std::move(info),
        windowId);
    return;
  }

  if (result == type::Result::NOT_FOUND && !visit_data.domain.empty()) {
    auto callback = std::bind(&Publisher::OnSaveVisitInternal,
                              this,
                              _1,
                              _2);

    SaveVisit(visit_data.domain, visit_data, 0, true, windowId, callback);
  }
}

void Publisher::GetPublisherBanner(
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  const auto banner_callback = std::bind(&Publisher::OnGetPublisherBanner,
                this,
                _1,
                publisher_key,
                callback);

  // NOTE: We do not attempt to search the prefix list before getting
  // the publisher data because if the prefix list was not properly
  // loaded then the user would not see the correct banner information
  // for a verified publisher. Assuming that the user has explicitly
  // requested this information by interacting with the UI, we should
  // make a best effort to return correct and updated information even
  // if the prefix list is incorrect.
  GetServerPublisherInfo(publisher_key, banner_callback);
}

void Publisher::OnGetPublisherBanner(
    type::ServerPublisherInfoPtr info,
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  auto banner = type::PublisherBanner::New();

  if (info) {
    if (info->banner) {
      banner = info->banner->Clone();
    }

    banner->status = info->status;
  }

  banner->publisher_key = publisher_key;

  const auto publisher_callback =
      std::bind(&Publisher::OnGetPublisherBannerPublisher,
                this,
                callback,
                *banner,
                _1,
                _2);

  ledger_->database()->GetPublisherInfo(publisher_key, publisher_callback);
}

void Publisher::OnGetPublisherBannerPublisher(
    ledger::PublisherBannerCallback callback,
    const type::PublisherBanner& banner,
    type::Result result,
    type::PublisherInfoPtr publisher_info) {
  auto new_banner = type::PublisherBanner::New(banner);

  if (!publisher_info || result != type::Result::LEDGER_OK) {
    BLOG(0, "Publisher info not found");
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
    client::GetServerPublisherInfoCallback callback) {
  ledger_->database()->GetServerPublisherInfo(
      publisher_key,
      std::bind(&Publisher::OnServerPublisherInfoLoaded,
          this,
          _1,
          publisher_key,
          callback));
}

void Publisher::OnServerPublisherInfoLoaded(
    type::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  if (ShouldFetchServerPublisherInfo(server_info.get())) {
    // Store the current server publisher info so that if fetching fails
    // we can execute the callback with the last known valid data.
    auto shared_info = std::make_shared<type::ServerPublisherInfoPtr>(
        std::move(server_info));

    FetchServerPublisherInfo(
        publisher_key,
        [shared_info, callback](type::ServerPublisherInfoPtr info) {
          callback(std::move(info ? info : *shared_info));
        });
    return;
  }

  callback(std::move(server_info));
}

void Publisher::UpdateMediaDuration(
    const uint64_t window_id,
    const std::string& publisher_key,
    const uint64_t duration,
    const bool first_visit) {
  BLOG(1, "Media duration: " << duration);
  ledger_->database()->GetPublisherInfo(publisher_key,
      std::bind(&Publisher::OnGetPublisherInfoForUpdateMediaDuration,
                this,
                _1,
                _2,
                window_id,
                duration,
                first_visit));
}

void Publisher::OnGetPublisherInfoForUpdateMediaDuration(
    type::Result result,
    type::PublisherInfoPtr info,
    const uint64_t window_id,
    const uint64_t duration,
    const bool first_visit) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to retrieve publisher info while updating media duration");
    return;
  }

  type::VisitData visit_data;
  visit_data.name = info->name;
  visit_data.url = info->url;
  visit_data.provider = info->provider;
  visit_data.favicon_url = info->favicon_url;

  SaveVideoVisit(
      info->id,
      visit_data,
      duration,
      first_visit,
      0,
      [](type::Result, type::PublisherInfoPtr) {});
}

void Publisher::GetPublisherPanelInfo(
    const std::string& publisher_key,
    ledger::GetPublisherInfoCallback callback) {
  auto filter = CreateActivityFilter(
      publisher_key,
      type::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->state()->GetReconcileStamp(),
      true,
      false);

  ledger_->database()->GetPanelPublisherInfo(std::move(filter),
      std::bind(&Publisher::OnGetPanelPublisherInfo,
                this,
                _1,
                _2,
                callback));
}

void Publisher::OnGetPanelPublisherInfo(
    const type::Result result,
    type::PublisherInfoPtr info,
    ledger::GetPublisherInfoCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to retrieve panel publisher info");
    callback(result, nullptr);
    return;
  }

  callback(result, std::move(info));
}

void Publisher::SavePublisherInfo(
    const uint64_t window_id,
    type::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  if (!publisher_info || publisher_info->id.empty()) {
    BLOG(0, "Publisher key is missing for url");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  type::VisitData visit_data;
  visit_data.provider = publisher_info->provider;
  visit_data.name = publisher_info->name;
  visit_data.url = publisher_info->url;
  if (!publisher_info->favicon_url.empty()) {
    visit_data.favicon_url = publisher_info->favicon_url;
  }

  auto banner_callback = std::bind(
      &Publisher::OnGetPublisherBannerForSavePublisherInfo,
      this,
      _1,
      window_id,
      publisher_info->id,
      visit_data,
      callback);

  GetPublisherBanner(publisher_info->id, banner_callback);
}

void Publisher::OnGetPublisherBannerForSavePublisherInfo(
    type::PublisherBannerPtr banner,
    const uint64_t window_id,
    const std::string& publisher_key,
    const type::VisitData& visit_data,
    ledger::ResultCallback callback) {
  type::VisitData new_visit_data = visit_data;

  if (banner && !banner->logo.empty()) {
    auto index = banner->logo.find("https://");
    if (index != std::string::npos) {
      new_visit_data.favicon_url = std::string(banner->logo, index);
    }
  }

  SaveVisit(
      publisher_key,
      new_visit_data,
      0,
      true,
      window_id,
      [callback](auto result, type::PublisherInfoPtr publisher_info) {
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
        base::StringPrintf(
            "https://twitter.com/%s/status/%s",
            name->second.c_str(),
            tweet_id->second.c_str());
    share_url = base::StringPrintf(
        "https://twitter.com/intent/tweet?text=%s&url=%s",
        comment_with_hashtag.c_str(),
        quoted_tweet_url.c_str());
  } else {
    share_url = base::StringPrintf(
        "https://twitter.com/intent/tweet?text=%s",
        comment_with_hashtag.c_str());
  }

  return share_url;
}

}  // namespace publisher
}  // namespace ledger
