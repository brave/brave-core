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

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/properties/publisher_settings_properties.h"
#include "bat/ledger/internal/properties/report_balance_properties.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/publisher/publisher_server_list.h"
#include "bat/ledger/internal/state/publisher_settings_state.h"
#include "bat/ledger/internal/static_values.h"

/* foo.bar.example.com
   QLD = 'bar'
   RLD = 'foo.bar'
   SLD = 'example.com'
   TLD = 'com'

  search.yahoo.co.jp
   QLD = 'search'
   RLD = 'search'
   SLD = 'yahoo.co.jp'
   TLD = 'co.jp'
*/

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_publisher {

Publisher::Publisher(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  state_(new ledger::PublisherSettingsProperties),
  server_list_(std::make_unique<PublisherServerList>(ledger)) {
  calcScoreConsts(state_->min_page_time_before_logging_a_visit);
}

Publisher::~Publisher() {
}

void Publisher::OnTimer(uint32_t timer_id) {
  server_list_->OnTimer(timer_id);
}

void Publisher::RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback) {
  server_list_->Download(std::bind(&Publisher::OnRefreshPublisher,
          this,
          _1,
          publisher_key,
          callback));
}

void Publisher::OnRefreshPublisher(
    const ledger::Result result,
    const std::string& publisher_key,
    ledger::OnRefreshPublisherCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(ledger::PublisherStatus::NOT_VERIFIED);
    return;
  }

  const auto server_callback =
    std::bind(&Publisher::OnRefreshPublisherServerPublisher,
              this,
              _1,
              callback);

  ledger_->GetServerPublisherInfo(publisher_key, server_callback);
}

void Publisher::OnRefreshPublisherServerPublisher(
    ledger::ServerPublisherInfoPtr info,
    ledger::OnRefreshPublisherCallback callback) {
  auto status = ledger::PublisherStatus::NOT_VERIFIED;
  if (info) {
    status = info->status;
  }
  callback(status);
}

void Publisher::SetPublisherServerListTimer() {
  server_list_->SetTimer(false);
}

void Publisher::calcScoreConsts(const uint64_t& min_duration_seconds) {
  // we increase duration for 100 to keep it as close to muon implementation
  // as possible (we used 1000 in muon)
  // keeping it with only seconds visits are not spaced out equally
  uint64_t min_duration_big = min_duration_seconds * 100;
  a_ = (1.0 / (braveledger_ledger::_d * 2.0)) - min_duration_big;
  a2_ = a_ * 2.0;
  a4_ = a2_ * 2.0;
  b_ = min_duration_big - a_;
  b2_ = b_ * b_;
}

// courtesy of @dimitry-xyz:
// https://github.com/brave/ledger/issues/2#issuecomment-221752002
double Publisher::concaveScore(const uint64_t& duration_seconds) {
  uint64_t duration_big = duration_seconds * 100;
  return (-b_ + std::sqrt(b2_ + (a4_ * duration_big))) / a2_;
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
  if (!ledger_->GetRewardsMainEnabled() || publisher_key.empty()) {
    return;
  }

  auto server_callback =
      std::bind(&Publisher::OnSaveVisitServerPublisher,
                this,
                _1,
                publisher_key,
                visit_data,
                duration,
                window_id,
                callback);

  ledger_->GetServerPublisherInfo(publisher_key, server_callback);
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
  filter->min_duration = min_duration ? getPublisherMinVisitTime() : 0;
  filter->reconcile_stamp = current_reconcile_stamp;
  filter->non_verified = non_verified;
  filter->min_visits = min_visits ? GetPublisherMinVisits() : 0;

  return filter;
}

std::string Publisher::GetBalanceReportName(
    const ledger::ActivityMonth month,
    int year) {
  return base::StringPrintf("%d_%d", year, month);
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

  bool server_excluded = server_info && server_info->excluded;

  ledger::PublisherInfoCallback callbackGetPublishers =
      std::bind(&Publisher::SaveVisitInternal,
          this,
          status,
          server_excluded,
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
    bool server_excluded,
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
                          "https://" + ledger_->GenerateGUID() + ".invalid",
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

  bool excluded = IsExcluded(
      publisher_info->id,
      server_excluded,
      static_cast<ledger::PublisherExclude>(publisher_info->excluded));
  bool ignore_time = ignoreMinTime(publisher_key);
  if (duration == 0) {
    ignore_time = false;
  }

  ledger::PublisherInfoPtr panel_info = nullptr;

  if (excluded) {
    publisher_info->excluded = ledger::PublisherExclude::EXCLUDED;
  }

  // for new visits that are excluded or are not long enough or ac is off
  bool min_duration_new = duration < getPublisherMinVisitTime() &&
      !ignore_time;
  bool min_duration_ok = duration > getPublisherMinVisitTime() || ignore_time;
  bool verified_new = !ledger_->GetPublisherAllowNonVerified() && !is_verified;
  bool verified_old = (
      (!ledger_->GetPublisherAllowNonVerified() && is_verified) ||
      ledger_->GetPublisherAllowNonVerified());

  if (new_visit &&
      (excluded ||
       !ledger_->GetAutoContribute() ||
       min_duration_new ||
       verified_new)) {
    panel_info = publisher_info->Clone();

    ledger_->SetPublisherInfo(std::move(publisher_info));
  } else if (!excluded &&
             ledger_->GetAutoContribute() &&
             min_duration_ok &&
             verified_old) {
    publisher_info->visits += 1;
    publisher_info->duration += duration;
    publisher_info->score += concaveScore(duration);
    publisher_info->reconcile_stamp = ledger_->GetReconcileStamp();

    panel_info = publisher_info->Clone();

    ledger_->SetActivityInfo(std::move(publisher_info));
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
    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Missing or corrupted favicon file for: " << publisher_key;
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
  if (result == ledger::Result::LEDGER_OK && !favicon_url.empty()) {
    info->favicon_url = favicon_url;

    ledger::PublisherInfoPtr panel_info = info->Clone();

    ledger_->SetPublisherInfo(std::move(info));

    if (window_id > 0) {
      ledger::VisitData visit_data;
      OnPanelPublisherInfo(ledger::Result::LEDGER_OK,
                          std::move(panel_info),
                          window_id,
                          visit_data);
    }
  } else {
    BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
      "Missing or corrupted favicon file";
  }
}

void Publisher::OnPublisherInfoSaved(
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  if (result != ledger::Result::LEDGER_OK || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Publisher info was not saved!";
  }

  SynopsisNormalizer();
}

void Publisher::SetPublisherExclude(
    const std::string& publisher_id,
    const ledger::PublisherExclude& exclude,
    ledger::SetPublisherExcludeCallback callback) {
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
    ledger::SetPublisherExcludeCallback callback) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    callback(result);
    return;
  }

  if (!publisher_info || publisher_info->excluded == exclude) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  publisher_info->excluded = exclude;
  ledger_->SetPublisherInfo(publisher_info->Clone());
  if (exclude == ledger::PublisherExclude::EXCLUDED) {
    ledger_->DeleteActivityInfo(
      publisher_info->id,
      [](ledger::Result _){});
  }
  callback(ledger::Result::LEDGER_OK);
}

void Publisher::OnRestorePublishers(
    const ledger::Result result,
    ledger::RestorePublishersCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Could not restore publishers.";
    callback(result);
    return;
  }

  SynopsisNormalizer();
  callback(ledger::Result::LEDGER_OK);
}

// In seconds
void Publisher::setPublisherMinVisitTime(const uint64_t& duration) {
  state_->min_page_time_before_logging_a_visit = duration;
  calcScoreConsts(duration);
  SynopsisNormalizer();
  saveState();
}

void Publisher::setPublisherMinVisits(const unsigned int visits) {
  state_->min_visits_for_publisher_relevancy = visits;
  SynopsisNormalizer();
  saveState();
}

void Publisher::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_sites_in_list = allow;
  SynopsisNormalizer();
  saveState();
}

void Publisher::setPublisherAllowVideos(const bool& allow) {
  state_->allow_contribution_to_videos = allow;
  SynopsisNormalizer();
  saveState();
}

uint64_t Publisher::getPublisherMinVisitTime() const {
  return state_->min_page_time_before_logging_a_visit;
}

unsigned int Publisher::GetPublisherMinVisits() const {
  return state_->min_visits_for_publisher_relevancy;
}

bool Publisher::getPublisherAllowNonVerified() const {
  return state_->allow_non_verified_sites_in_list;
}

bool Publisher::getPublisherAllowVideos() const {
  return state_->allow_contribution_to_videos;
}

bool Publisher::GetMigrateScore() const {
  return state_->migrate_score_2;
}

void Publisher::SetMigrateScore(bool value) {
  state_->migrate_score_2 = value;
  saveState();
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
  if (list->size() == 0) {
    return;
  }

  double totalScores = 0.0;
  for (size_t i = 0; i < list->size(); i++) {
    // Check which would test uint problem from this issue
    // https://github.com/brave/brave-browser/issues/3134
    if (GetMigrateScore()) {
      (*list)[i]->score = concaveScore((*list)[i]->duration);
    }
    totalScores += (*list)[i]->score;
  }

  if (GetMigrateScore()) {
    SetMigrateScore(false);
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
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  // TODO(SZ): We pull the whole list currently,
  // I don't think it consumes lots of RAM, but could.
  // We need to limit it and iterate.
  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Publisher::SynopsisNormalizerCallback, this, _1, _2));
}

void Publisher::SynopsisNormalizerCallback(
    ledger::PublisherInfoList list,
    uint32_t record) {
  ledger::PublisherInfoList normalized_list;
  synopsisNormalizerInternal(&normalized_list, &list, 0);
  ledger_->SaveNormalizedPublisherList(std::move(normalized_list));
}

bool Publisher::IsConnectedOrVerified(const ledger::PublisherStatus status) {
  return status == ledger::PublisherStatus::CONNECTED ||
         status == ledger::PublisherStatus::VERIFIED;
}

bool Publisher::IsExcluded(
    const std::string& publisher_id,
    const bool server_exclude,
    const ledger::PublisherExclude& excluded) {
  // If exclude is set to 1, we should avoid further computation and return true
  if (excluded == ledger::PublisherExclude::EXCLUDED) {
    return true;
  }

  if (excluded == ledger::PublisherExclude::INCLUDED) {
    return false;
  }

  return server_exclude;
}

void Publisher::clearAllBalanceReports() {
  if (state_->monthly_balances.empty()) {
    return;
  }
  state_->monthly_balances.clear();
  saveState();
}

void Publisher::setBalanceReport(ledger::ActivityMonth month,
                                int year,
                                const ledger::BalanceReportInfo& report_info) {
  ledger::ReportBalanceProperties report_balance;
  report_balance.grants = report_info.grants;
  report_balance.ad_earnings = report_info.earning_from_ads;
  report_balance.recurring_donations = report_info.recurring_donation;
  report_balance.one_time_donations = report_info.one_time_donation;
  report_balance.auto_contributions = report_info.auto_contribute;

  state_->monthly_balances[GetBalanceReportName(month, year)] = report_balance;
  saveState();
}

void Publisher::GetBalanceReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetBalanceReportCallback callback) {
  ledger::BalanceReportInfo info;
  const auto result = GetBalanceReportInternal(month, year, &info);
  callback(result, info.Clone());
}

ledger::Result Publisher::GetBalanceReportInternal(
    const ledger::ActivityMonth month,
    const int year,
    ledger::BalanceReportInfo* report_info) {
  if (!report_info) {
    return ledger::Result::LEDGER_ERROR;
  }

  const std::string name = GetBalanceReportName(month, year);
  auto iter = state_->monthly_balances.find(name);

  if (iter == state_->monthly_balances.end()) {
    ledger::BalanceReportInfo new_report_info;
    new_report_info.grants = 0.0;
    new_report_info.earning_from_ads = 0.0;
    new_report_info.auto_contribute = 0.0;
    new_report_info.recurring_donation = 0.0;
    new_report_info.one_time_donation = 0.0;

    setBalanceReport(month, year, new_report_info);
    ledger::Result result = GetBalanceReportInternal(month, year, report_info);
    if (result == ledger::Result::LEDGER_OK) {
      iter = state_->monthly_balances.find(name);
    } else {
      return ledger::Result::LEDGER_ERROR;
    }
  }

  report_info->grants = iter->second.grants;
  report_info->earning_from_ads = iter->second.ad_earnings;
  report_info->auto_contribute = iter->second.auto_contributions;
  report_info->recurring_donation = iter->second.recurring_donations;
  report_info->one_time_donation = iter->second.one_time_donations;

  return ledger::Result::LEDGER_OK;
}

std::map<std::string, ledger::BalanceReportInfoPtr>
Publisher::GetAllBalanceReports() {
  std::map<std::string, ledger::BalanceReportInfoPtr> newReports;
  for (auto const& report : state_->monthly_balances) {
    ledger::BalanceReportInfoPtr newReport = ledger::BalanceReportInfo::New();
    const ledger::ReportBalanceProperties oldReport = report.second;
    newReport->grants = oldReport.grants;
    newReport->earning_from_ads = oldReport.ad_earnings;
    newReport->auto_contribute = oldReport.auto_contributions;
    newReport->recurring_donation = oldReport.recurring_donations;
    newReport->one_time_donation = oldReport.one_time_donations;

    newReports[report.first] = std::move(newReport);
  }

  return newReports;
}

void Publisher::saveState() {
  const ledger::PublisherSettingsState publisher_settings_state;
  const std::string data = publisher_settings_state.ToJson(*state_);
  ledger_->SavePublisherState(data, this);
}

bool Publisher::loadState(const std::string& data) {
  ledger::PublisherSettingsProperties state;
  const ledger::PublisherSettingsState publisher_settings_state;
  if (!publisher_settings_state.FromJson(data.c_str(), &state))
    return false;

  state_.reset(new ledger::PublisherSettingsProperties(state));
  calcScoreConsts(state_->min_page_time_before_logging_a_visit);
  return true;
}

void Publisher::OnPublisherStateSaved(ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not save publisher state";
    // TODO(anyone) error handling
    return;
  }
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
  }

  if (result == ledger::Result::NOT_FOUND && !visit_data.domain.empty()) {
    auto callback = std::bind(&Publisher::OnSaveVisitInternal,
                              this,
                              _1,
                              _2);

    SaveVisit(visit_data.domain, visit_data, 0, windowId, callback);
  }
}

void Publisher::SetBalanceReportItem(
    const ledger::ActivityMonth month,
    const int year,
    const ledger::ReportType type,
    const double amount) {
  ledger::BalanceReportInfo report_info;
  GetBalanceReportInternal(month, year, &report_info);

  switch (type) {
    case ledger::ReportType::GRANT_UGP:
      report_info.grants = report_info.grants + amount;
      break;
    case ledger::ReportType::GRANT_AD:
      report_info.earning_from_ads = report_info.earning_from_ads + amount;
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      report_info.auto_contribute = report_info.auto_contribute + amount;
      break;
    case ledger::ReportType::TIP:
      report_info.one_time_donation = report_info.one_time_donation + amount;
      break;
    case ledger::ReportType::TIP_RECURRING:
      report_info.recurring_donation = report_info.recurring_donation + amount;
      break;
    default:
      break;
  }

  setBalanceReport(month, year, report_info);
}

void Publisher::GetPublisherBanner(
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  const auto banner_callback = std::bind(&Publisher::OnGetPublisherBanner,
                this,
                _1,
                publisher_key,
                callback);

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

void Publisher::SavePublisherProcessed(const std::string& publisher_key) {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  if (std::find(list.begin(), list.end(), publisher_key) == list.end()) {
    state_->processed_pending_publishers.push_back(publisher_key);
  }
  saveState();
}

bool Publisher::WasPublisherAlreadyProcessed(
    const std::string& publisher_key) const {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  return std::find(list.begin(), list.end(), publisher_key) != list.end();
}

}  // namespace braveledger_publisher
