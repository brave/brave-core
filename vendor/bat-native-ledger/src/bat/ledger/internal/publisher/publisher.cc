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
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/prefix_util.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/publisher/publisher_prefix_list_updater.h"
#include "bat/ledger/internal/publisher/server_publisher_fetcher.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/state/state_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_publisher {

Publisher::Publisher(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger),
    prefix_list_updater_(
        std::make_unique<PublisherPrefixListUpdater>(ledger)),
    server_publisher_fetcher_(
        std::make_unique<ServerPublisherFetcher>(ledger)) {
}

Publisher::~Publisher() {
}

bool Publisher::ShouldFetchServerPublisherInfo(
    ledger::ServerPublisherInfo* server_info) {
  return server_publisher_fetcher_->IsExpired(server_info);
}

void Publisher::FetchServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  return server_publisher_fetcher_->Fetch(publisher_key, callback);
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
            : ledger::PublisherStatus::NOT_VERIFIED;

        // If, after refresh, the publisher is now verified
        // attempt to process any pending contributions for
        // unverified publishers.
        if (status == ledger::PublisherStatus::VERIFIED) {
          ledger_->ContributeUnverifiedPublishers();
        }

        callback(status);
      });
}

void Publisher::SetPublisherServerListTimer(const bool rewards_enabled) {
  if (rewards_enabled) {
    prefix_list_updater_->StartAutoUpdate(
        std::bind(&Publisher::OnPublisherPrefixListUpdated, this));
  } else {
    prefix_list_updater_->StopAutoUpdate();
  }
}

void Publisher::CalcScoreConsts(const int min_duration_seconds) {
  // we increase duration for 100 to keep it as close to muon implementation
  // as possible (we used 1000 in muon)
  // keeping it with only seconds visits are not spaced out equally
  uint64_t min_duration_big = min_duration_seconds * 100;
  const double d = 1.0 / (30.0 * 1000.0);
  const double a = (1.0 / (d * 2.0)) - min_duration_big;
  const double b = min_duration_big - a;

  braveledger_state::SetScoreValues(ledger_, a, b);
}

// courtesy of @dimitry-xyz:
// https://github.com/brave/ledger/issues/2#issuecomment-221752002
double Publisher::concaveScore(const uint64_t& duration_seconds) {
  uint64_t duration_big = duration_seconds * 100;
  double a, b;
  braveledger_state::GetScoreValues(ledger_, &a, &b);
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
    const ledger::VisitData& visit_data,
    const uint64_t& duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback) {
  if (!ledger_->GetRewardsMainEnabled()) {
    return;
  }

  if (publisher_key.empty()) {
    BLOG(0, "Publisher key is empty");
    return;
  }

  auto on_server_info = std::bind(&Publisher::OnSaveVisitServerPublisher,
      this, _1, publisher_key, visit_data, duration, window_id, callback);

  ledger_->SearchPublisherPrefixList(
      publisher_key,
      [this, publisher_key, on_server_info](bool publisher_exists) {
        if (publisher_exists) {
          ledger_->GetServerPublisherInfo(publisher_key, on_server_info);
        } else {
          on_server_info(nullptr);
        }
      });
}

ledger::ActivityInfoFilterPtr Publisher::CreateActivityFilter(
    const std::string& publisher_id,
    ledger::ExcludeFilter excluded,
    bool min_duration,
    const uint64_t& current_reconcile_stamp,
    bool non_verified,
    bool min_visits) {
  auto filter = ledger::ActivityInfoFilter::New();
  filter->id = publisher_id;
  filter->excluded = excluded;
  filter->min_duration = min_duration
      ? braveledger_state::GetPublisherMinVisitTime(ledger_)
      : 0;
  filter->reconcile_stamp = current_reconcile_stamp;
  filter->non_verified = non_verified;
  filter->min_visits = min_visits
      ? braveledger_state::GetPublisherMinVisits(ledger_)
      : 0;

  return filter;
}

void Publisher::OnSaveVisitServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const ledger::VisitData& visit_data,
    uint64_t duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback) {
  auto filter = CreateActivityFilter(
      publisher_key,
      ledger::ExcludeFilter::FILTER_ALL,
      false,
      ledger_->GetReconcileStamp(),
      true,
      false);

  // we need to do this as I can't move server publisher into final function
  auto status = ledger::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  ledger::PublisherInfoCallback callbackGetPublishers =
      std::bind(&Publisher::SaveVisitInternal,
          this,
          status,
          publisher_key,
          visit_data,
          duration,
          window_id,
          callback,
          _1,
          _2);

  ledger_->GetActivityInfo(std::move(filter), callbackGetPublishers);
}

void Publisher::SaveVisitInternal(
    const ledger::PublisherStatus status,
    const std::string& publisher_key,
    const ledger::VisitData& visit_data,
    uint64_t duration,
    uint64_t window_id,
    const ledger::PublisherInfoCallback callback,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  DCHECK(result != ledger::Result::TOO_MANY_RESULTS);
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(0, "Visit was not saved " << result);
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bool is_verified = ledger_->IsPublisherConnectedOrVerified(status);

  bool new_visit = false;
  if (!publisher_info) {
    new_visit = true;
    publisher_info = ledger::PublisherInfo::New();
    publisher_info->id = publisher_key;
  }

  std::string fav_icon = visit_data.favicon_url;
  if (is_verified && !fav_icon.empty()) {
    if (fav_icon.find(".invalid") == std::string::npos) {
    ledger_->FetchFavIcon(fav_icon,
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
    publisher_info->favicon_url = ledger::kClearFavicon;
  }

  publisher_info->name = visit_data.name;
  publisher_info->provider = visit_data.provider;
  publisher_info->url = visit_data.url;
  publisher_info->status = status;

  bool excluded =
      publisher_info->excluded == ledger::PublisherExclude::EXCLUDED;
  bool ignore_time = ignoreMinTime(publisher_key);
  if (duration == 0) {
    ignore_time = false;
  }

  ledger::PublisherInfoPtr panel_info = nullptr;

  uint64_t min_visit_time = static_cast<uint64_t>(
      braveledger_state::GetPublisherMinVisitTime(ledger_));

  // for new visits that are excluded or are not long enough or ac is off
  bool min_duration_new = duration < min_visit_time && !ignore_time;
  bool min_duration_ok = duration > min_visit_time || ignore_time;
  bool verified_new =
      !braveledger_state::GetPublisherAllowNonVerified(ledger_) && !is_verified;
  bool verified_old =
      (!braveledger_state::GetPublisherAllowNonVerified(ledger_) &&
          is_verified) ||
      braveledger_state::GetPublisherAllowNonVerified(ledger_);

  if (new_visit &&
      (excluded ||
       !ledger_->GetAutoContributeEnabled() ||
       min_duration_new ||
       verified_new)) {
    panel_info = publisher_info->Clone();

    auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
        this,
        _1);

    ledger_->SavePublisherInfo(std::move(publisher_info), callback);
  } else if (!excluded &&
             ledger_->GetAutoContributeEnabled() &&
             min_duration_ok &&
             verified_old) {
    publisher_info->visits += 1;
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = ledger_->GetReconcileStamp();

    panel_info = publisher_info->Clone();

    auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
        this,
        _1);

    ledger_->SaveActivityInfo(std::move(publisher_info), callback);
  }

  if (panel_info) {
    if (panel_info->favicon_url == ledger::kClearFavicon) {
      panel_info->favicon_url = std::string();
    }

    auto callback_info = panel_info->Clone();
    callback(ledger::Result::LEDGER_OK, std::move(callback_info));

    if (window_id > 0) {
      OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
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
    BLOG(1, "Missing or corrupted favicon file for: " << publisher_key);
    return;
  }

  ledger_->GetPublisherInfo(publisher_key,
      std::bind(&Publisher::onFetchFavIconDBResponse,
                this,
                _1,
                _2,
                favicon_url,
                window_id));
}

void Publisher::onFetchFavIconDBResponse(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    const std::string& favicon_url,
    uint64_t window_id) {
  if (result != ledger::Result::LEDGER_OK || favicon_url.empty()) {
    BLOG(1, "Missing or corrupted favicon file");
    return;
  }

  info->favicon_url = favicon_url;

  auto callback = std::bind(&Publisher::OnPublisherInfoSaved,
      this,
      _1);

  ledger_->SavePublisherInfo(info->Clone(), callback);

  if (window_id > 0) {
    ledger::VisitData visit_data;
    OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
                        std::move(info),
                        window_id,
                        visit_data);
  }
}

void Publisher::OnPublisherInfoSaved(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Publisher info was not saved!");
    return;
  }

  SynopsisNormalizer();
}

void Publisher::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PublisherExclude& exclude,
    ledger::ResultCallback callback) {
  ledger_->GetPublisherInfo(
    publisher_id,
    std::bind(&Publisher::OnSetPublisherExclude,
              this,
              exclude,
              _1,
              _2,
              callback));
}

void Publisher::OnSetPublisherExclude(
    ledger::PublisherExclude exclude,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    BLOG(0, "Publisher exclude status not saved");
    callback(result);
    return;
  }

  if (!publisher_info) {
    BLOG(0, "Publisher is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (publisher_info->excluded == exclude) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  publisher_info->excluded = exclude;

  auto save_callback = std::bind(&Publisher::OnPublisherInfoSaved,
      this,
      _1);
  ledger_->SavePublisherInfo(publisher_info->Clone(), save_callback);
  if (exclude == ledger::PublisherExclude::EXCLUDED) {
    ledger_->DeleteActivityInfo(
      publisher_info->id,
      [](const ledger::Result _){});
  }
  callback(ledger::Result::LEDGER_OK);
}

void Publisher::OnRestorePublishers(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Could not restore publishers.");
    callback(result);
    return;
  }

  SynopsisNormalizer();
  callback(ledger::Result::LEDGER_OK);
}

void Publisher::NormalizeContributeWinners(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList* list,
    uint32_t record) {
  synopsisNormalizerInternal(newList, list, record);
}

void Publisher::synopsisNormalizerInternal(
    ledger::PublisherInfoList* newList,
    const ledger::PublisherInfoList* list,
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
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp(),
      braveledger_state::GetPublisherAllowNonVerified(ledger_),
      braveledger_state::GetPublisherMinVisits(ledger_));
  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Publisher::SynopsisNormalizerCallback, this, _1));
}

void Publisher::SynopsisNormalizerCallback(
    ledger::PublisherInfoList list) {
  ledger::PublisherInfoList normalized_list;
  synopsisNormalizerInternal(&normalized_list, &list, 0);
  ledger_->SaveNormalizedPublisherList(std::move(normalized_list));
}

bool Publisher::IsConnectedOrVerified(const ledger::PublisherStatus status) {
  return status == ledger::PublisherStatus::CONNECTED ||
         status == ledger::PublisherStatus::VERIFIED;
}

void Publisher::getPublisherActivityFromUrl(
    uint64_t windowId,
    const ledger::VisitData& visit_data,
    const std::string& publisher_blob) {
  if (!ledger_->GetRewardsMainEnabled()) {
    return;
  }

  const bool is_media = visit_data.domain == YOUTUBE_TLD ||
                        visit_data.domain == TWITCH_TLD ||
                        visit_data.domain == TWITTER_TLD ||
                        visit_data.domain == REDDIT_TLD ||
                        visit_data.domain == VIMEO_TLD ||
                        visit_data.domain == GITHUB_TLD;

  if (is_media &&
      visit_data.path != "" && visit_data.path != "/") {
    std::string type = YOUTUBE_MEDIA_TYPE;
    if (visit_data.domain == TWITCH_TLD) {
      type = TWITCH_MEDIA_TYPE;
    } else if (visit_data.domain == TWITTER_TLD) {
      type = TWITTER_MEDIA_TYPE;
    } else if (visit_data.domain == REDDIT_TLD) {
      type = REDDIT_MEDIA_TYPE;
    } else if (visit_data.domain == VIMEO_TLD) {
      type = VIMEO_MEDIA_TYPE;
    } else if (visit_data.domain == GITHUB_TLD) {
      type = GITHUB_MEDIA_TYPE;
    }

    ledger::VisitDataPtr new_visit_data = ledger::VisitData::New(visit_data);

    if (!new_visit_data->url.empty()) {
      new_visit_data->url.pop_back();
    }

    new_visit_data->url += new_visit_data->path;

    ledger_->GetMediaActivityFromUrl(windowId,
                                     std::move(new_visit_data),
                                     type,
                                     publisher_blob);
    return;
  }

  auto filter = CreateActivityFilter(visit_data.domain,
        ledger::ExcludeFilter::FILTER_ALL,
        false,
        ledger_->GetReconcileStamp(),
        true,
        false);

  ledger::VisitData new_data;
  new_data.domain = visit_data.domain;
  new_data.path = visit_data.path;
  new_data.name = visit_data.name;
  new_data.url = visit_data.url;
  new_data.favicon_url = "";

  ledger_->GetPanelPublisherInfo(std::move(filter),
        std::bind(&Publisher::OnPanelPublisherInfo,
                  this,
                  _1,
                  _2,
                  windowId,
                  new_data));
}

void Publisher::OnSaveVisitInternal(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  // TODO(nejczdovc): handle if needed
}

void Publisher::OnPanelPublisherInfo(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t windowId,
    const ledger::VisitData& visit_data) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->OnPanelPublisherInfo(result, std::move(info), windowId);
    return;
  }

  if (result == ledger::Result::NOT_FOUND && !visit_data.domain.empty()) {
    auto callback = std::bind(&Publisher::OnSaveVisitInternal,
                              this,
                              _1,
                              _2);

    SaveVisit(visit_data.domain, visit_data, 0, windowId, callback);
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
  ledger_->GetServerPublisherInfo(publisher_key, banner_callback);
}

void Publisher::OnGetPublisherBanner(
    ledger::ServerPublisherInfoPtr info,
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  auto banner = ledger::PublisherBanner::New();

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

  ledger_->GetPublisherInfo(publisher_key, publisher_callback);
}

void Publisher::OnGetPublisherBannerPublisher(
    ledger::PublisherBannerCallback callback,
    const ledger::PublisherBanner& banner,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  auto new_banner = ledger::PublisherBanner::New(banner);

  if (!publisher_info || result != ledger::Result::LEDGER_OK) {
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

void Publisher::OnPublisherPrefixListUpdated() {
  // Attempt to reprocess any contributions for previously
  // unverified publishers that are now verified.
  ledger_->ContributeUnverifiedPublishers();

  // Remove stale server publisher records to recover space.
  server_publisher_fetcher_->PurgeExpiredRecords();
}

}  // namespace braveledger_publisher
